#include "camera/camera_t205.hpp"
#include "camera/camera_zr10.hpp"
#include "camera/gimbal_transform.hpp"
#include "core/angles.hpp"
#include "core/crc.hpp"
#include "core/emath.hpp"
#include "core/kalman.hpp"
#include "core/pixel_mapper.hpp"
#include "controller/client_controller.hpp"
#include "controller/vehicle_controller.hpp"
#include "elink/elink_abstract_handler.hpp"
#include "elink/elink_arm_handler.hpp"
#include "elink/elink_attitude_handler.hpp"
#include "elink/elink_communicator.hpp"
#include "elink/elink_flower_handler.hpp"
#include "elink/elink_gimbal_handler.hpp"
#include "elink/elink_interceptor_handler.hpp"
#include "elink/elink_movement_handler.hpp"
#include "elink/elink_position_handler.hpp"
#include "elink/elink_shumodav_handler.hpp"
#include "elink/elink_status_handler.hpp"
#include "led/vehicle_led.hpp"
#include "interception/einterceptor.hpp"
#include "logging/elogger.hpp"
#include "link/tcp_link.hpp"
#include "link/serial_link.hpp"
#include "link/udp_link.hpp"
#include "pathfinder/pathfinder.hpp"
#include "scout/escout.hpp"
#include "remote/remote_controller.hpp"
#include "road/road_types.hpp"
#include "road/road_runner.hpp"
#include "road/roads.hpp"
#include "vnav/vnav.hpp"
#include "vision/thunder_gaze.hpp"
#include "vision/vision.hpp"

#ifdef EPATHFINDER_HAS_MAVLINK
#include "elink/elink_telemetry.hpp"
#include "mavlink/attitude_handler.hpp"
#include "mavlink/control_handler.hpp"
#include "mavlink/gps_handler.hpp"
#include "mavlink/heartbeat_handler.hpp"
#include "mavlink/log_handler.hpp"
#include "mavlink/mavlink_communicator.hpp"
#include "mavlink/plan_handler.hpp"
#include "mavlink/rc_channels_handler.hpp"
#include "mavlink/time_sync_handler.hpp"
#include "mavlink/wind_handler.hpp"
#endif

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QThread>
#include <QTimerEvent>
#include <QUdpSocket>
#include <QtEndian>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <type_traits>

#ifdef EPATHFINDER_HAS_LIVOX_SDK
#include "lidar/interceptor_livox.hpp"
#include "lidar/lds_lidar.hpp"

#include <livox_sdk.h>
#endif

struct RemoteControllerTestProbe {
    static void receive(RemoteController& controller,
                        const QByteArray& bytes,
                        const qlonglong time)
    {
        controller.parseBytes(bytes, time);
    }
};

struct EInterceptorTestProbe {
    static void receive(EInterceptor& interceptor,
                        const QByteArray& bytes,
                        const qlonglong time)
    {
        interceptor.parseBytes(bytes, time);
    }
};

struct ClientControllerTestProbe {
    static void parse(ClientController& controller, QByteArray& bytes)
    {
        controller.parseBuffer(bytes);
    }
};

#ifdef EPATHFINDER_HAS_MAVLINK
#include <ardupilotmega/mavlink.h>
#endif

#ifdef EPATHFINDER_HAS_MAVLINK
struct ELinkTelemetryTestProbe {
    static QByteArray queued(const ELinkTelemetry& telemetry,
                             const unsigned char type)
    {
        for (const auto& message : telemetry.messages_) {
            if (message.type == type) {
                return message.data;
            }
        }
        return {};
    }

    static int queueSize(const ELinkTelemetry& telemetry)
    {
        return telemetry.messages_.size();
    }
};
#endif

#ifdef EPATHFINDER_HAS_LIVOX_SDK
struct LdsLidarTestProbe {
    static void resetPacketCount(LdsLidar& lidar,
                                 const std::uint8_t handle,
                                 const std::uint32_t value = 0U)
    {
        lidar.data_recveive_count_[handle] = value;
    }

    static void receive(LdsLidar& lidar,
                        const std::uint8_t handle,
                        LivoxEthPacket* const packet,
                        const std::uint32_t pointCount)
    {
        LdsLidar::GetLidarDataCb(handle, packet, pointCount, &lidar);
    }
};

struct InterceptorLivoxTestProbe {
    static void receive(InterceptorLivox& interceptor,
                        const QList<LivoxPoint>& points)
    {
        interceptor.livoxDataCallback(points);
    }
};
#endif

namespace {

int failures = 0;

void check(const bool condition, const char* expression, const int line)
{
    if (!condition) {
        std::cerr << "line " << line << ": failed: " << expression << '\n';
        ++failures;
    }
}

#define CHECK(expression) check((expression), #expression, __LINE__)

bool near(const double actual, const double expected, const double tolerance)
{
    return std::abs(actual - expected) <= tolerance;
}

template <typename T>
void appendTestLittleEndian(QByteArray& output, const T value)
{
    static_assert(std::is_trivially_copyable_v<T>);
    const auto* bytes = reinterpret_cast<const char*>(&value);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    output.append(bytes, static_cast<qsizetype>(sizeof(T)));
#else
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        output.append(bytes[sizeof(T) - i - 1U]);
    }
#endif
}

template <typename T>
T readTestLittleEndian(const QByteArray& input, const qsizetype offset)
{
    static_assert(std::is_trivially_copyable_v<T>);
    T value{};
    CHECK(offset >= 0);
    CHECK(input.size() - offset >= static_cast<qsizetype>(sizeof(T)));
    if (offset < 0
        || input.size() - offset < static_cast<qsizetype>(sizeof(T))) {
        return value;
    }
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    std::memcpy(&value, input.constData() + offset, sizeof(T));
#else
    char bytes[sizeof(T)];
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        bytes[i] = input[offset + sizeof(T) - i - 1U];
    }
    std::memcpy(&value, bytes, sizeof(T));
#endif
    return value;
}

bool receiveUdpDatagram(QUdpSocket& socket,
                        QByteArray& datagram,
                        const int timeoutMs = 1000)
{
    if (!socket.hasPendingDatagrams()
        && !socket.waitForReadyRead(timeoutMs)) {
        return false;
    }
    datagram.resize(static_cast<qsizetype>(socket.pendingDatagramSize()));
    return socket.readDatagram(datagram.data(), datagram.size())
        == datagram.size();
}

template <typename Predicate>
bool processEventsUntil(Predicate&& predicate, const int timeoutMs = 500)
{
    QElapsedTimer elapsed;
    elapsed.start();
    do {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        if (predicate()) {
            return true;
        }
        QThread::msleep(1);
    } while (elapsed.elapsed() < timeoutMs);
    return predicate();
}

class TestVNav final : public VNav {
public:
    using VNav::VNav;

    void runTimerEvent()
    {
        QTimerEvent event(0);
        timerEvent(&event);
    }
};

class TestELink final : public AbstractLink {
public:
    bool isUp() const override { return up_; }
    void up() override { up_ = true; }
    void down() override { up_ = false; }
    void sendData(const QByteArray& data) override
    {
        transmissions.append(data);
    }
    void sendData(const char* const data, const qlonglong length) override
    {
        transmissions.append(
            QByteArray(data, static_cast<qsizetype>(length)));
    }
    void feed(const QByteArray& data) { emit dataReceived(data); }

    bool up_{true};
    QList<QByteArray> transmissions;
};

class TestELinkHandler final : public ELinkAbstractHandler {
public:
    using ELinkAbstractHandler::ELinkAbstractHandler;

    int processCount{};
    std::uint8_t uint8Value{};
    std::int16_t int16Value{};
    std::uint16_t uint16Value{};
    std::uint32_t uint32Value{};
    int intValue{};
    float floatValue{};

protected:
    void processMessage() override
    {
        ++processCount;
        readUInt8(2, uint8Value);
        readInt16(3, int16Value);
        readUInt16(5, uint16Value);
        readUInt32(7, uint32Value);
        readInt(11, intValue);
        readFloat(15, floatValue);
    }
};

QByteArray makeTestELinkFrame(const std::uint8_t id,
                              const QByteArray& body,
                              const std::uint8_t sequence = 0U)
{
    QByteArray payload;
    payload.append(static_cast<char>(sequence));
    payload.append(static_cast<char>(id));
    payload.append(body);
    const std::uint16_t checksum = crc16(
        reinterpret_cast<const std::uint8_t*>(payload.constData()),
        static_cast<std::uint16_t>(payload.size()));
    payload.append(static_cast<char>(checksum & 0xffU));
    payload.append(static_cast<char>((checksum >> 8U) & 0xffU));

    QByteArray frame = QByteArray::fromHex("d00d");
    appendTestLittleEndian(frame,
                           static_cast<std::uint16_t>(payload.size()));
    frame.append(payload);
    return frame;
}

void checkTestELinkFrame(const QByteArray& frame,
                         const std::uint8_t sequence,
                         const std::uint8_t id,
                         const QByteArray& body)
{
    CHECK(frame.size() == 8 + body.size());
    CHECK(frame.left(2) == QByteArray::fromHex("d00d"));
    CHECK(readTestLittleEndian<std::uint16_t>(frame, 2)
          == static_cast<std::uint16_t>(body.size() + 4));
    CHECK(static_cast<std::uint8_t>(frame[4]) == sequence);
    CHECK(static_cast<std::uint8_t>(frame[5]) == id);
    CHECK(frame.mid(6, body.size()) == body);
    const auto expectedCrc = crc16(
        reinterpret_cast<const std::uint8_t*>(frame.constData() + 4),
        static_cast<std::uint16_t>(body.size() + 2));
    CHECK(readTestLittleEndian<std::uint16_t>(frame, 6 + body.size())
          == expectedCrc);
}

void testCrc()
{
    std::array<std::uint8_t, 9> input{
        '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    CHECK(crc16(input.data(), static_cast<std::uint16_t>(input.size()))
          == 0x29b1U);
    CHECK(crc16(input.data(), 0U) == 0xffffU);
}

void testAngles()
{
    CHECK(near(averageAngle({}), 0.0, 0.0));
    CHECK(near(averageAngle({10.0F}), 10.0, 0.0));
    CHECK(near(averageAngle({10.0F, 20.0F, 30.0F}), 20.0, 1.0e-6));
    CHECK(near(averageAngle({0.25F, 0.25F}), 0.0, 0.0));
    CHECK(near(averageAngle({350.0F, 10.0F}), 0.0, 1.0e-6));
}

void testIntersection()
{
    const QVector3D point = rayPlaneIntersection(
        {0.0F, 0.0F, 10.0F},
        {0.0F, 0.0F, 1.0F},
        {1.0F, 2.0F, 0.0F},
        {0.0F, 0.0F, 2.0F});
    CHECK(near(point.x(), 1.0, 1.0e-6));
    CHECK(near(point.y(), 2.0, 1.0e-6));
    CHECK(near(point.z(), 10.0, 1.0e-6));
    CHECK(rayPlaneIntersection({0, 0, 1}, {0, 0, 1}, {0, 0, 0}, {1, 0, 0})
              .isNull());
}

void testKalman()
{
    Kalman filter;
    CHECK(near(filter.q(), 0.02, 1.0e-7));
    CHECK(near(filter.r(), 0.10, 1.0e-7));
    CHECK(near(filter.kalman(1.0F), 0.9901039, 1.0e-5));
    filter.setQ(0.03F);
    filter.setR(0.20F);
    CHECK(near(filter.q(), 0.03, 1.0e-7));
    CHECK(near(filter.r(), 0.20, 1.0e-7));
}

void testPixelMapper()
{
    CHECK(near(lin(5.0, 0.0, 10.0, 10.0, 30.0), 20.0, 1.0e-12));

    double zoom = -1.0;
    double lower = -1.0;
    double upper = -1.0;
    calculate_zoom(zoom, lower, upper);
    CHECK(zoom == 0.0 && lower == 0.0 && upper == 5.0);

    zoom = 12.0;
    calculate_zoom(zoom, lower, upper);
    CHECK(zoom == 12.0 && lower == 10.0 && upper == 15.0);

    CHECK(selectZoomMap(2.0) == ZoomMap::Zoom00);
    CHECK(selectZoomMap(2.0001) == ZoomMap::Zoom05);
    CHECK(selectZoomMap(17.0) == ZoomMap::Zoom15);
    CHECK(selectZoomMap(17.0001) == ZoomMap::Zoom20);
    CHECK(get_zoom_data(0.0) == zoom_00);
    CHECK(get_zoom_data(8.0) == zoom_10);
    CHECK(get_zoom_data(20.0) == zoom_20);
    CHECK(sizeof(zoom_00) == kZoomMapBytes);

    QTemporaryDir directory;
    CHECK(directory.isValid());
    QFile file(directory.filePath(QStringLiteral("sample.dat")));
    CHECK(file.open(QIODevice::WriteOnly));
    CHECK(file.write("ABCD", 4) == 4);
    file.close();
    std::array<char, 4> output{};
    CHECK(read_data(file.fileName(), output.data(), 4));
    const std::array<char, 4> expected{{'A', 'B', 'C', 'D'}};
    CHECK(output == expected);

    const QMatrix4x4 constructed = createQMatrix4x4(1.0,
                                                     2.0,
                                                     3.0,
                                                     4.0,
                                                     5.0,
                                                     6.0,
                                                     7.0,
                                                     8.0,
                                                     9.0);
    CHECK(constructed(0, 0) == 1.0F);
    CHECK(constructed(0, 2) == 3.0F);
    CHECK(constructed(2, 0) == 7.0F);
    CHECK(constructed(2, 2) == 9.0F);
    CHECK(constructed(3, 3) == 1.0F);

    float calibration[3][3]{{800.0F, 0.0F, 960.0F},
                             {0.0F, 810.0F, 540.0F},
                             {0.0F, 0.0F, 1.0F}};
    const QMatrix4x4 transform =
        get_transform(calibration, 2.0, 10.0, 20.0, 30.0);
    // Values captured by emulating RVA 0x4a0c8. Both QMatrix4x4::inverted
    // calls were modeled as binary32 column-major inversions.
    CHECK(near(transform(0, 0), -2.2064100e-3, 2.0e-8));
    CHECK(near(transform(0, 1), -4.4514356e-5, 2.0e-8));
    CHECK(near(transform(0, 2), 1.2024990, 2.0e-6));
    CHECK(near(transform(1, 0), -1.1024240e-3, 2.0e-8));
    CHECK(near(transform(1, 1), 9.3462301e-4, 2.0e-8));
    CHECK(near(transform(1, 2), 2.1812260, 2.0e-6));
    CHECK(near(transform(2, 0), -2.0396989e-4, 2.0e-8));
    CHECK(near(transform(2, 1), -1.1424896e-3, 2.0e-8));
    CHECK(near(transform(2, 2), 1.1547756, 2.0e-6));

    const QPoint samplePixel(3, 4);
    const std::size_t sampleIndex =
        static_cast<std::size_t>(samplePixel.y()) * kZoomMapWidth
        + static_cast<std::size_t>(samplePixel.x());
    const auto initializeMap = [sampleIndex](float* map,
                                              const float mapX,
                                              const float mapY) {
        const std::array<float, 9> identity{{1.0F, 0.0F, 0.0F,
                                             0.0F, 1.0F, 0.0F,
                                             0.0F, 0.0F, 1.0F}};
        std::copy(identity.begin(), identity.end(), map);
        map[kZoomMatrixElements + sampleIndex] = mapX;
        map[kZoomMatrixElements + kZoomPlaneElements + sampleIndex] = mapY;
    };
    initializeMap(zoom_00, 2.0F, 4.0F);
    initializeMap(zoom_05, 4.0F, 2.0F);
    const QVector<QPoint> points{samplePixel};
    const QVector<QPointF> projected =
        reproject_impl(0.0, 1.0, 0.0, 0.0, 0.0, points);
    CHECK(projected.size() == 1);
    CHECK(near(projected[0].x(), 0.5, 1.0e-7));
    CHECK(near(projected[0].y(), -0.25, 1.0e-7));

    init = true;
    const QVector<QPointF> interpolated =
        reproject(2.5, 1.0, 0.0, 0.0, 0.0, points);
    CHECK(interpolated.size() == 1);
    CHECK(near(interpolated[0].x(), 1.25, 1.0e-7));
    CHECK(near(interpolated[0].y(), -0.375, 1.0e-7));
    init = false;
    CHECK(reproject(2.5, 1.0, 0.0, 0.0, 0.0, points).isEmpty());
}

void testGimbalRoundTrip()
{
    const QQuaternion matrix = gimbalMTX({10.0F, 20.0F, 30.0F});
    // Components captured by emulating RVA 0x49840 with the three Qt
    // fromAxisAndAngle calls modeled at binary32 precision.
    CHECK(near(matrix.scalar(), 0.94371432, 2.0e-7));
    CHECK(near(matrix.x(), 0.18930776, 2.0e-7));
    CHECK(near(matrix.y(), 0.26853588, 2.0e-7));
    CHECK(near(matrix.z(), 0.03813458, 2.0e-7));

    const RPY body{2.0F, -3.0F, 15.0F};
    const RPY gimbal{1.0F, 4.0F, -8.0F};
    const RPY recovered = camera2body(body2camera(body, gimbal), gimbal);
    CHECK(near(recovered.roll, body.roll, 1.0e-4));
    CHECK(near(recovered.pitch, body.pitch, 1.0e-4));
    CHECK(near(recovered.yaw, body.yaw, 1.0e-4));
}

void testCameraRecovery()
{
    static_assert(sizeof(Zoom) == 0x20U);
    static_assert(offsetof(Zoom, opticalZoom) == 0x00U);
    static_assert(offsetof(Zoom, percent) == 0x04U);
    static_assert(offsetof(Zoom, widthAngle) == 0x08U);
    static_assert(offsetof(Zoom, heightAngle) == 0x0cU);
    static_assert(offsetof(Zoom, scaleX) == 0x10U);
    static_assert(offsetof(Zoom, scaleY) == 0x14U);
    static_assert(offsetof(Zoom, time) == 0x18U);
    static_assert(sizeof(Gimbal) == 0x58U);
    static_assert(offsetof(Gimbal, time) == 0x00U);
    static_assert(offsetof(Gimbal, autoCenter) == 0x08U);
    static_assert(offsetof(Gimbal, roll) == 0x0cU);
    static_assert(offsetof(Gimbal, yaw) == 0x10U);
    static_assert(offsetof(Gimbal, pitch) == 0x14U);
    static_assert(offsetof(Gimbal, center) == 0x3fU);
    static_assert(offsetof(Gimbal, gimbalId) == 0x41U);
    static_assert(offsetof(Gimbal, yawByNorth) == 0x42U);
    static_assert(offsetof(CameraParams, gimbal) == 0x08U);
    static_assert(offsetof(CameraParams, zoom) == 0x78U);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    static_assert(sizeof(CameraParams) == 0xa0U);
    static_assert(offsetof(CameraParams, calibrations) == 0x98U);
#endif

    CameraT205 t205(QString{});
    const CameraParams t205Params = t205.currentParams();
    CHECK(t205Params.zoomControlEnabled);
    CHECK(t205Params.externalGimbalControl);
    CHECK(t205Params.calibrations.size() == 21);
    CHECK(t205Params.calibrations.first().opticalZoom == 0.0F);
    CHECK(t205Params.calibrations.first().percent == 0.0F);
    CHECK(near(t205Params.calibrations.first().scaleX,
               1.016700029373169, 0.0));
    CHECK(near(t205Params.calibrations.first().scaleY,
               1.065000057220459, 0.0));
    CHECK(t205Params.calibrations.last().opticalZoom == 736.0F);
    CHECK(t205Params.calibrations.last().percent == 100.0F);

    QUdpSocket receiver;
    CHECK(receiver.bind(QHostAddress::LocalHost, 0));
    const int remotePort = receiver.localPort();
    CameraZR10 zr10(QStringLiteral("127.0.0.1"),
                    remotePort == 65535 ? remotePort - 1 : remotePort + 1,
                    remotePort,
                    false);
    const CameraParams zr10Params = zr10.currentParams();
    CHECK(zr10Params.zoomControlEnabled);
    CHECK(!zr10Params.externalGimbalControl);
    CHECK(zr10Params.calibrations.size() == 24);
    CHECK(zr10Params.calibrations.first().opticalZoom == 1.5F);
    CHECK(zr10Params.calibrations.first().percent == 0.0F);
    CHECK(zr10Params.calibrations.last().opticalZoom == 30.0F);
    CHECK(zr10Params.calibrations.last().percent == 100.0F);

    std::array<unsigned char, 9> crcInput{
        '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    CHECK(zr10.CRC16_cal(crcInput.data(), crcInput.size(), 0) == 0x31c3U);

    zr10.setZoom(100.0F);
    QByteArray packet;
    CHECK(receiveUdpDatagram(receiver, packet));
    CHECK(packet.size() == 12);
    CHECK(packet.left(10) == QByteArray::fromHex("556601020000000f1e00"));
    const auto packetCrc = zr10.CRC16_cal(
        reinterpret_cast<unsigned char*>(packet.data()), 10, 0);
    CHECK(readTestLittleEndian<std::uint16_t>(packet, 10) == packetCrc);
    CHECK(zr10.zoom().opticalZoom == 30.0F);
    CHECK(zr10.zoom().percent == 100.0F);
}

void testLivoxVersion()
{
#ifdef EPATHFINDER_HAS_LIVOX_SDK
    LivoxSdkVersion version{};
    GetLivoxSdkVersion(&version);
    CHECK(version.major == 2U);
    CHECK(version.minor == 3U);
    CHECK(version.patch == 0U);
#endif
}

void testLivoxDataPath()
{
#ifdef EPATHFINDER_HAS_LIVOX_SDK
    static_assert(sizeof(LivoxPoint) == 13U);
    static_assert(sizeof(LivoxExtendRawPoint) == 14U);
    static_assert(sizeof(PlaneCoord) == 12U);

    InterceptorLivox interceptor;
    CHECK(interceptor.frameTime() == 100);
    CHECK(interceptor.metaObject()->indexOfMethod("Init()") >= 0);
    CHECK(interceptor.metaObject()->indexOfMethod("setFrameTime(int)") >= 0);
    interceptor.setFrameTime(250);
    CHECK(interceptor.frameTime() == 250);

    QList<QList<PlaneCoord>> emissions;
    QObject::connect(
        &interceptor,
        &InterceptorLidar::lidarData,
        [&](const QList<PlaneCoord>& objects) { emissions.append(objects); });

    InterceptorLivoxTestProbe::receive(interceptor, {});
    CHECK(emissions.size() == 1);
    CHECK(emissions[0].isEmpty());

    const QList<LivoxPoint> clusterInput{
        LivoxPoint{0.0F, 0.0F, 0.0F, 1U},
        LivoxPoint{9.0F, 0.0F, 0.0F, 2U},
        LivoxPoint{17.0F, 0.0F, 0.0F, 3U},
        LivoxPoint{10.0F, 0.0F, 0.0F, 4U},
    };
    InterceptorLivoxTestProbe::receive(interceptor, clusterInput);
    CHECK(emissions.size() == 2);
    CHECK(emissions[1].size() == 2);
    CHECK(near(emissions[1][0].x, 4.5, 0.0));
    CHECK(near(emissions[1][0].y, 0.0, 0.0));
    CHECK(near(emissions[1][0].z, 0.0, 0.0));
    CHECK(near(emissions[1][1].x, 13.5, 0.0));

    auto& source = LdsLidar::GetInstance();
    QList<LivoxPoint> converted;
    int callbackCount = 0;
    source.SetQDataCallback([&](const QList<LivoxPoint>& points) {
        ++callbackCount;
        converted = points;
    });

    constexpr std::size_t packetBytes =
        offsetof(LivoxEthPacket, data)
        + 2U * sizeof(LivoxExtendRawPoint);
    alignas(LivoxEthPacket) std::array<std::byte, packetBytes> storage{};
    auto* const packet = reinterpret_cast<LivoxEthPacket*>(storage.data());
    packet->data_type = kExtendCartesian;
    auto* const raw =
        reinterpret_cast<LivoxExtendRawPoint*>(packet->data);
    raw[0] = LivoxExtendRawPoint{1000, -2000, 3000, 7U, 8U};
    raw[1] = LivoxExtendRawPoint{0, 0, 0, 9U, 10U};

    LdsLidarTestProbe::resetPacketCount(source, 0U);
    for (int packetIndex = 0; packetIndex < 99; ++packetIndex) {
        LdsLidarTestProbe::receive(source, 0U, packet, 2U);
    }
    CHECK(callbackCount == 0);
    LdsLidarTestProbe::receive(source, 0U, packet, 2U);
    CHECK(callbackCount == 1);
    CHECK(converted.size() == 1);
    CHECK(near(converted[0].x, 1.0, 0.0));
    CHECK(near(converted[0].y, -2.0, 0.0));
    CHECK(near(converted[0].z, 3.0, 0.0));
    CHECK(converted[0].reflectivity == 7U);

    packet->data_type = kCartesian;
    LdsLidarTestProbe::resetPacketCount(source, 0U, 99U);
    LdsLidarTestProbe::receive(source, 0U, packet, 2U);
    CHECK(callbackCount == 1);
#endif
}

void testVehicleLed()
{
    VehicleLed led;
    led.setPin(257);
    led.init(VehicleLed::State::Off);
    led.setState(VehicleLed::State::On);
    led.setState(VehicleLed::State::BlinkInitiallyOn);
    led.setState(VehicleLed::State::Off);

    GPIOController controller(1, 2, 3, 4, 5);
    controller.Init();
    controller.setLedState(VehicleLed::Type::Type0, VehicleLed::State::On);
    controller.setLedState(VehicleLed::Type::Type1, VehicleLed::State::Off);
}

void testMavlinkDialect()
{
#ifdef EPATHFINDER_HAS_MAVLINK
    static_assert(sizeof(mavlink_message_t) == 291U);
    CHECK(MAVLINK_PRIMARY_XML_HASH == 4884480585825823206ULL);
    CHECK(QString::fromLatin1(MAVLINK_BUILD_DATE)
          == QStringLiteral("Thu Jul 04 2024"));

    const mavlink_msg_entry_t* const entry = mavlink_get_msg_entry(52005U);
    CHECK(entry != nullptr);
    CHECK(entry->crc_extra == 145U);
    CHECK(entry->min_msg_len == 1U);
    CHECK(entry->max_msg_len == 1U);
#endif
}

#ifdef EPATHFINDER_HAS_MAVLINK
class TestLink final : public AbstractLink {
public:
    explicit TestLink(const QString& name)
    {
        linkName_ = name;
    }

    bool isUp() const override { return isUp_; }
    void up() override { isUp_ = true; }
    void down() override { isUp_ = false; }
    void reconnect() override { isUp_ = true; }

    void sendData(const QByteArray& data) override { transmissions.push_back(data); }

    void sendData(const char* const data, const qlonglong length) override
    {
        transmissions.push_back(QByteArray(data, static_cast<qsizetype>(length)));
    }

    void feed(const QByteArray& data) { emit dataReceived(data); }

    bool isUp_{true};
    QList<QByteArray> transmissions;
};

QByteArray serializeMavlink(const mavlink_message_t& message)
{
    std::array<std::uint8_t, MAVLINK_MAX_PACKET_LEN> bytes{};
    const std::uint16_t length =
        mavlink_msg_to_send_buffer(bytes.data(), &message);
    return QByteArray(reinterpret_cast<const char*>(bytes.data()), length);
}

mavlink_message_t parseMavlink(const QByteArray& frame,
                               const mavlink_channel_t channel)
{
    mavlink_message_t message{};
    mavlink_status_t status{};
    for (const char byte : frame) {
        if (mavlink_parse_char(channel,
                               static_cast<uint8_t>(byte),
                               &message,
                               &status)
            != 0) {
            return message;
        }
    }
    return {};
}

void testMavlinkCommunicator()
{
    MavLinkCommunicator communicator(42U, 17U);
    CHECK(communicator.systemId() == 42U);
    CHECK(communicator.componentId() == 17U);
    communicator.setSystemId(43U);
    communicator.setComponentId(18U);
    CHECK(communicator.systemId() == 43U);
    CHECK(communicator.componentId() == 18U);
    CHECK(communicator.metaObject()->indexOfMethod(
              "addLink(AbstractLink*,uint8_t,int)")
          >= 0);

    TestLink mainLink(QStringLiteral("main"));
    TestLink backupLink(QStringLiteral("backup"));
    communicator.addLink(&mainLink, MAVLINK_COMM_0, 7);
    communicator.addLink(&backupLink, MAVLINK_COMM_1, 300);
    communicator.addLink(&backupLink, MAVLINK_COMM_2, 1);
    CHECK(communicator.links().size() == 2);
    CHECK(communicator.links().contains(&mainLink));
    CHECK(communicator.links().contains(&backupLink));

    mavlink_message_t message{};
    mavlink_msg_heartbeat_pack(43U,
                               18U,
                               &message,
                               MAV_TYPE_GENERIC,
                               MAV_AUTOPILOT_GENERIC,
                               MAV_MODE_FLAG_TEST_ENABLED,
                               0x12345678U,
                               MAV_STATE_ACTIVE);
    const QByteArray frame = serializeMavlink(message);

    communicator.sendMessageOnMainLink(message);
    CHECK(mainLink.transmissions.size() == 1);
    CHECK(mainLink.transmissions.back() == frame);
    communicator.sendMessageOnBackupLink(message);
    CHECK(backupLink.transmissions.size() == 1);
    CHECK(backupLink.transmissions.back() == frame);
    communicator.sendMessageOnAllLinks(message);
    CHECK(mainLink.transmissions.size() == 2);
    CHECK(backupLink.transmissions.size() == 2);

    int primaryCount = 0;
    int backupCount = 0;
    QString backupSender;
    unsigned char backupId = 0;
    std::uint32_t lastMessageId = UINT32_MAX;
    QObject::connect(
        &communicator,
        &MavLinkCommunicator::messageReceived,
        [&](const mavlink_message_t& received) {
            ++primaryCount;
            lastMessageId = received.msgid;
        });
    QObject::connect(
        &communicator,
        &MavLinkCommunicator::backupMessageReceived,
        [&](const QString& sender,
            const mavlink_message_t& received,
            const unsigned char id) {
            ++backupCount;
            backupSender = sender;
            backupId = id;
            lastMessageId = received.msgid;
        });

    mainLink.feed(frame);
    CHECK(primaryCount == 1);
    CHECK(backupCount == 0);
    CHECK(lastMessageId == MAVLINK_MSG_ID_HEARTBEAT);
    communicator.sendMessageOnLastReceivedLink(message);
    CHECK(mainLink.transmissions.size() == 3);

    backupLink.feed(frame);
    CHECK(primaryCount == 1);
    CHECK(backupCount == 1);
    CHECK(backupSender == QStringLiteral("backup"));
    CHECK(backupId == static_cast<unsigned char>(300));
    CHECK(lastMessageId == MAVLINK_MSG_ID_HEARTBEAT);

    communicator.removeLink(&backupLink);
    CHECK(communicator.links().size() == 1);
    backupLink.feed(frame);
    CHECK(backupCount == 1);
}

void testMavlinkPlanHandler()
{
    MavLinkCommunicator communicator(51U, 52U);
    TestLink link(QStringLiteral("plan"));
    communicator.addLink(&link, MAVLINK_COMM_2, 4);

    PlanHandler handler(&communicator);
    CHECK(handler.metaObject()->indexOfMethod(
              "writePlanAnswer(QJsonDocument)")
          >= 0);
    CHECK(handler.metaObject()->indexOfMethod(
              "processMessage(mavlink_message_t)")
          >= 0);
    CHECK(handler.metaObject()->indexOfMethod("setArmDisarm(bool)") >= 0);

    QList<QJsonDocument> answers;
    int writtenEvents = 0;
    QObject::connect(&handler,
                     &PlanHandler::writePlanAnswer,
                     [&](const QJsonDocument& answer) {
                         answers.append(answer);
                     });
    QObject::connect(&handler,
                     &PlanHandler::planWrited,
                     [&] { ++writtenEvents; });

    handler.setPlanVersion(3);
    const QByteArray imageData("abc", 3);
    QJsonObject image;
    image.insert(QStringLiteral("img"),
                 QString::fromLatin1(imageData.toBase64()));
    image.insert(QStringLiteral("name"), QStringLiteral("sample"));
    image.insert(QStringLiteral("crc"),
                 QStringLiteral("900150983cd24fb0d6963f7d28e17f72"));
    image.insert(QStringLiteral("x"), -123);
    image.insert(QStringLiteral("y"), 456);
    image.insert(QStringLiteral("dir"), 78);
    image.insert(QStringLiteral("h"), 901);

    QJsonObject wind;
    wind.insert(QStringLiteral("direction"), 271);
    wind.insert(QStringLiteral("speed"), 12);
    QJsonObject node;
    node.insert(QStringLiteral("number"), 9);
    node.insert(QStringLiteral("lat"), 50.1);
    node.insert(QStringLiteral("lon"), 30.2);
    node.insert(QStringLiteral("alt"), 345.5);
    node.insert(QStringLiteral("radius"), 100);
    node.insert(QStringLiteral("timeout"), -5);
    node.insert(QStringLiteral("activate"), 1);
    node.insert(QStringLiteral("type"), 2);
    node.insert(QStringLiteral("road_type"), 5);
    node.insert(QStringLiteral("trust_gps"), 1);
    node.insert(QStringLiteral("wind"), wind);

    QJsonObject roadNode;
    roadNode.insert(QStringLiteral("number"), 4);
    roadNode.insert(QStringLiteral("lat"), 50.3);
    roadNode.insert(QStringLiteral("lng"), 30.4);
    QJsonObject roadLink;
    roadLink.insert(QStringLiteral("from"), 4);
    roadLink.insert(QStringLiteral("to"), 5);
    roadLink.insert(QStringLiteral("category"), 7);

    QJsonObject data;
    data.insert(QStringLiteral("wind_attack"), 1);
    data.insert(QStringLiteral("nodes"), QJsonArray{node});
    data.insert(QStringLiteral("road_nodes"), QJsonArray{roadNode});
    data.insert(QStringLiteral("road_links"), QJsonArray{roadLink});
    data.insert(QStringLiteral("target_images"), QJsonArray{image});
    QJsonObject root;
    root.insert(QStringLiteral("cookie"), QStringLiteral("write-cookie"));
    root.insert(QStringLiteral("data"), data);
    handler.writePlan(QJsonDocument(root));

    CHECK(answers.size() == 1);
    CHECK(answers.back().object().value(QStringLiteral("cookie")).toString()
          == QStringLiteral("write-cookie"));
    CHECK(answers.back().object().value(QStringLiteral("result")).toInt()
          == 0);
    CHECK(writtenEvents == 1);
    CHECK(!handler.transactionInProgress());

    const Plan plan = handler.currentPlan();
    CHECK(plan.version == 3);
    CHECK(plan.items.size() == 2);
    CHECK(plan.items[0].number == -1);
    CHECK(plan.items[1].number == 9);
    CHECK(plan.items[1].radius == 500);
    CHECK(plan.items[1].timeout == 0);
    CHECK(plan.items[1].windAttack);
    CHECK(plan.items[1].trustGps);
    CHECK(plan.items[1].windDirection == 271);
    CHECK(plan.items[1].windSpeed == 12.0F);
    CHECK(plan.hasRailway);
    CHECK(plan.roadPoints.size() == 1);
    CHECK(plan.roadPoints[0].number == 5);
    CHECK(plan.roadPoints[0].longitude == 30.4);
    CHECK(plan.roadLinks.size() == 1);
    CHECK(plan.roadLinks[0].from == 5);
    CHECK(plan.roadLinks[0].to == 6);
    CHECK(plan.targetImages.size() == 1);
    CHECK(plan.targetImages[0].image == imageData);
    CHECK(plan.targetImages[0].x == -123);
    CHECK(plan.targetImages[0].height == 901);

    handler.getPlan(QJsonDocument(
        QJsonObject{{QStringLiteral("cookie"),
                     QStringLiteral("read-cached")}}));
    CHECK(answers.size() == 2);
    const QJsonObject cached = answers.back().object();
    CHECK(cached.value(QStringLiteral("count")).toInt() == 1);
    CHECK(cached.value(QStringLiteral("cookie")).toString()
          == QStringLiteral("read-cached"));
    const QJsonObject cachedNode =
        cached.value(QStringLiteral("nodes")).toArray()[0].toObject();
    CHECK(cachedNode.value(QStringLiteral("msl")).toDouble() == 136.0);
    CHECK(cachedNode.value(QStringLiteral("radius")).toInt() == 500);
    CHECK(cachedNode.value(QStringLiteral("trust_gps")).toInt() == 1);
    CHECK(!cachedNode.contains(QStringLiteral("wind")));

    qsizetype transmissionCount = link.transmissions.size();
    handler.startWritePlan();
    CHECK(link.transmissions.size() == transmissionCount + 1);
    mavlink_message_t generated =
        parseMavlink(link.transmissions.back(), MAVLINK_COMM_2);
    CHECK(generated.msgid == MAVLINK_MSG_ID_MISSION_COUNT);
    mavlink_mission_count_t uploadCount{};
    mavlink_msg_mission_count_decode(&generated, &uploadCount);
    CHECK(uploadCount.count == 2U);
    CHECK(uploadCount.target_system == 1U);
    CHECK(uploadCount.target_component == 1U);

    transmissionCount = link.transmissions.size();
    handler.setArmDisarm(true);
    CHECK(link.transmissions.size() == transmissionCount + 1);
    generated = parseMavlink(link.transmissions.back(), MAVLINK_COMM_2);
    CHECK(generated.msgid == MAVLINK_MSG_ID_COMMAND_LONG);
    mavlink_command_long_t arm{};
    mavlink_msg_command_long_decode(&generated, &arm);
    CHECK(generated.compid == MAV_COMP_ID_ONBOARD_COMPUTER);
    CHECK(arm.command == MAV_CMD_COMPONENT_ARM_DISARM);
    CHECK(arm.confirmation == 6U);
    CHECK(arm.param1 == 1.0F);

    // Failed image validation is caught as QString and returned as result 2.
    QJsonObject badRoot = root;
    QJsonObject badData = data;
    QJsonObject badImage = image;
    badImage.insert(QStringLiteral("crc"), QStringLiteral("invalid"));
    badData.insert(QStringLiteral("target_images"), QJsonArray{badImage});
    badRoot.insert(QStringLiteral("data"), badData);
    handler.writePlan(QJsonDocument(badRoot));
    CHECK(answers.size() == 3);
    CHECK(answers.back().object().value(QStringLiteral("result")).toInt()
          == 2);
    CHECK(answers.back().object().value(QStringLiteral("text")).toString()
              .contains(QStringLiteral("crc error")));
    CHECK(!handler.transactionInProgress());

    // A fresh handler exercises the remote read transaction.
    PlanHandler reader(&communicator);
    QList<QJsonDocument> readAnswers;
    QObject::connect(&reader,
                     &PlanHandler::writePlanAnswer,
                     [&](const QJsonDocument& answer) {
                         readAnswers.append(answer);
                     });
    transmissionCount = link.transmissions.size();
    reader.getPlan(QJsonDocument(
        QJsonObject{{QStringLiteral("cookie"),
                     QStringLiteral("remote")}}));
    CHECK(reader.transactionInProgress());
    CHECK(link.transmissions.size() == transmissionCount + 1);
    generated = parseMavlink(link.transmissions.back(), MAVLINK_COMM_2);
    CHECK(generated.msgid == MAVLINK_MSG_ID_MISSION_REQUEST_LIST);

    mavlink_message_t incoming{};
    mavlink_msg_mission_count_pack(1U,
                                   1U,
                                   &incoming,
                                   51U,
                                   52U,
                                   2U,
                                   MAV_MISSION_TYPE_MISSION,
                                   0U);
    reader.processMessage(incoming);
    generated = parseMavlink(link.transmissions.back(), MAVLINK_COMM_2);
    mavlink_mission_request_int_t request{};
    mavlink_msg_mission_request_int_decode(&generated, &request);
    CHECK(generated.msgid == MAVLINK_MSG_ID_MISSION_REQUEST_INT);
    CHECK(request.seq == 0U);

    mavlink_msg_mission_item_int_pack(1U,
                                      1U,
                                      &incoming,
                                      51U,
                                      52U,
                                      0U,
                                      MAV_FRAME_GLOBAL,
                                      MAV_CMD_NAV_WAYPOINT,
                                      0U,
                                      1U,
                                      0.0F,
                                      0.0F,
                                      0.0F,
                                      0.0F,
                                      500000000,
                                      300000000,
                                      100.0F,
                                      MAV_MISSION_TYPE_MISSION);
    reader.processMessage(incoming);
    generated = parseMavlink(link.transmissions.back(), MAVLINK_COMM_2);
    mavlink_msg_mission_request_int_decode(&generated, &request);
    CHECK(request.seq == 1U);

    mavlink_msg_mission_item_int_pack(1U,
                                      1U,
                                      &incoming,
                                      51U,
                                      52U,
                                      1U,
                                      MAV_FRAME_GLOBAL,
                                      MAV_CMD_NAV_WAYPOINT,
                                      0U,
                                      1U,
                                      0.0F,
                                      0.0F,
                                      0.0F,
                                      0.0F,
                                      501234567,
                                      301234567,
                                      222.5F,
                                      MAV_MISSION_TYPE_MISSION);
    reader.processMessage(incoming);
    generated = parseMavlink(link.transmissions.back(), MAVLINK_COMM_2);
    CHECK(generated.msgid == MAVLINK_MSG_ID_MISSION_ACK);
    CHECK(!reader.transactionInProgress());
    CHECK(readAnswers.size() == 1);
    const QJsonObject remote = readAnswers[0].object();
    CHECK(remote.value(QStringLiteral("count")).toInt() == 1);
    const QJsonObject remoteNode =
        remote.value(QStringLiteral("nodes")).toArray()[0].toObject();
    CHECK(near(remoteNode.value(QStringLiteral("lat")).toDouble(),
               50.1234567,
               1.0e-9));
    CHECK(near(remoteNode.value(QStringLiteral("lon")).toDouble(),
               30.1234567,
               1.0e-9));
    CHECK(remoteNode.value(QStringLiteral("alt")).toDouble() == 222.5);
}

void testMavlinkHandlers()
{
    MavLinkCommunicator communicator(51U, 52U);
    TestLink link(QStringLiteral("telemetry"));
    communicator.addLink(&link, MAVLINK_COMM_3, 9);

    HeartbeatHandler heartbeatHandler(7U, &communicator, 1.75F);
    CHECK(heartbeatHandler.metaObject()->indexOfMethod(
              "gimbalDeviceAttitudeStatus(qlonglong,uchar,uchar,float,float,float,float,float,float,float,float,float)")
          >= 0);
    CHECK(heartbeatHandler.metaObject()->indexOfMethod(
              "writeParam(QString,float)")
          >= 0);
    int modeEvents = 0;
    int observedMode = 0;
    int missionEvents = 0;
    int observedMission = -1;
    int compassEvents = 0;
    bool observedCompass = false;
    int homingOnEventsFromHeartbeat = 0;
    int armedEvents = 0;
    int launchEvents = 0;
    int validVersionEvents = 0;
    int airspeedEvents = 0;
    int distanceEvents = 0;
    int gimbalEvents = 0;
    int parameterEvents = 0;
    int backupBatteryEvents = 0;
    float observedAirspeed = 0.0F;
    float observedGroundspeed = 0.0F;
    unsigned int observedMinimumDistance = 0;
    unsigned int observedMaximumDistance = 0;
    unsigned int observedCurrentDistance = 0;
    QString observedParameter;
    float observedParameterValue = 0.0F;
    unsigned char observedBatteryId = 0;
    short observedBatteryCurrent = 0;
    QObject::connect(&heartbeatHandler,
                     &HeartbeatHandler::currentModeChanged,
                     [&](const int mode) {
                         ++modeEvents;
                         observedMode = mode;
                     });
    QObject::connect(&heartbeatHandler,
                     &HeartbeatHandler::currentMissionItemChanged,
                     [&](const int item) {
                         ++missionEvents;
                         observedMission = item;
                     });
    QObject::connect(&heartbeatHandler,
                     &HeartbeatHandler::compassStatusChanged,
                     [&](const bool enabled) {
                         ++compassEvents;
                         observedCompass = enabled;
                     });
    QObject::connect(&heartbeatHandler,
                     &HeartbeatHandler::homingOn,
                     [&] { ++homingOnEventsFromHeartbeat; });
    QObject::connect(&heartbeatHandler,
                     &HeartbeatHandler::armed,
                     [&] { ++armedEvents; });
    QObject::connect(&heartbeatHandler,
                     &HeartbeatHandler::launchAcceleration,
                     [&] { ++launchEvents; });
    QObject::connect(&heartbeatHandler,
                     &HeartbeatHandler::isValidArdupilotVersion,
                     [&] { ++validVersionEvents; });
    QObject::connect(
        &heartbeatHandler,
        &HeartbeatHandler::airspeedEvent,
        [&](const float airspeed, const float groundspeed) {
            ++airspeedEvents;
            observedAirspeed = airspeed;
            observedGroundspeed = groundspeed;
        });
    QObject::connect(
        &heartbeatHandler,
        &HeartbeatHandler::distanceSensorEvent,
        [&](const unsigned int minimum,
            const unsigned int maximum,
            const unsigned int current) {
            ++distanceEvents;
            observedMinimumDistance = minimum;
            observedMaximumDistance = maximum;
            observedCurrentDistance = current;
        });
    QObject::connect(
        &heartbeatHandler,
        &HeartbeatHandler::gimbalDeviceAttitudeStatus,
        [&](qlonglong,
            unsigned char,
            unsigned char,
            float,
            float,
            float,
            float,
            float,
            float,
            float,
            float,
            float) { ++gimbalEvents; });
    QObject::connect(
        &heartbeatHandler,
        &HeartbeatHandler::mavlinkParamValue,
        [&](const QString& name, const float value) {
            ++parameterEvents;
            observedParameter = name;
            observedParameterValue = value;
        });
    QObject::connect(
        &heartbeatHandler,
        &HeartbeatHandler::backupBattery,
        [&](const unsigned char id, const short current) {
            ++backupBatteryEvents;
            observedBatteryId = id;
            observedBatteryCurrent = current;
        });

    mavlink_message_t message{};
    mavlink_msg_heartbeat_pack(1U,
                               1U,
                               &message,
                               MAV_TYPE_FIXED_WING,
                               MAV_AUTOPILOT_ARDUPILOTMEGA,
                               0U,
                               73U,
                               MAV_STATE_ACTIVE);
    heartbeatHandler.processMessage(message);
    CHECK(modeEvents == 1);
    CHECK(observedMode == 73);

    mavlink_sys_status_t systemStatus{};
    systemStatus.onboard_control_sensors_present =
        MAV_SYS_STATUS_SENSOR_3D_MAG;
    systemStatus.onboard_control_sensors_enabled =
        MAV_SYS_STATUS_SENSOR_3D_MAG;
    systemStatus.onboard_control_sensors_health =
        MAV_SYS_STATUS_SENSOR_3D_MAG | MAV_SYS_STATUS_PREARM_CHECK;
    mavlink_msg_sys_status_encode(1U, 1U, &message, &systemStatus);
    heartbeatHandler.processMessage(message);
    CHECK(compassEvents == 1);
    CHECK(observedCompass);

    mavlink_msg_statustext_pack(1U,
                                1U,
                                &message,
                                MAV_SEVERITY_INFO,
                                "Homing On",
                                0U,
                                0U);
    heartbeatHandler.processMessage(message);
    CHECK(homingOnEventsFromHeartbeat == 1);
    mavlink_msg_statustext_pack(1U,
                                1U,
                                &message,
                                MAV_SEVERITY_INFO,
                                "Throttle armed",
                                0U,
                                0U);
    heartbeatHandler.processMessage(message);
    CHECK(armedEvents == 1);

    mavlink_mission_current_t mission{};
    mission.seq = 123U;
    mavlink_msg_mission_current_encode(1U, 1U, &message, &mission);
    heartbeatHandler.processMessage(message);
    CHECK(missionEvents == 1);
    CHECK(observedMission == 123);

    heartbeatHandler.setAccDelay(0U);
    heartbeatHandler.readAcceleration(true);
    mavlink_raw_imu_t rawImu{};
    rawImu.time_usec = 123456789ULL;
    rawImu.xacc = 5001;
    rawImu.yacc = 0;
    rawImu.zacc = 0;
    mavlink_msg_raw_imu_encode(1U, 1U, &message, &rawImu);
    heartbeatHandler.processMessage(message);
    QCoreApplication::processEvents();
    CHECK(near(heartbeatHandler.lastAcceleration(), 5001.0, 0.0));
    CHECK(launchEvents == 1);
    qlonglong rawImuSnapshotTime = -1;
    short rawValues[9]{-1, -1, -1, -1, -1, -1, -1, -1, -1};
    heartbeatHandler.rawIMULastValues(rawImuSnapshotTime,
                                      rawValues[0],
                                      rawValues[1],
                                      rawValues[2],
                                      rawValues[3],
                                      rawValues[4],
                                      rawValues[5],
                                      rawValues[6],
                                      rawValues[7],
                                      rawValues[8]);
    CHECK(rawImuSnapshotTime == 0);
    CHECK(std::all_of(std::begin(rawValues),
                      std::end(rawValues),
                      [](const short value) { return value == 0; }));

    mavlink_optical_flow_t opticalFlow{};
    opticalFlow.flow_x = -2;
    opticalFlow.flow_y = 321;
    mavlink_msg_optical_flow_encode(1U, 1U, &message, &opticalFlow);
    heartbeatHandler.processMessage(message);
    CHECK(heartbeatHandler.lastFlowXValue()
          == static_cast<std::uint16_t>(-2));
    CHECK(heartbeatHandler.lastFlowYValue() == 321U);

    mavlink_vfr_hud_t hud{};
    hud.airspeed = 12.5F;
    hud.groundspeed = 8.25F;
    mavlink_msg_vfr_hud_encode(1U, 1U, &message, &hud);
    heartbeatHandler.processMessage(message);
    CHECK(airspeedEvents == 1);
    CHECK(observedAirspeed == 12.5F);
    CHECK(observedGroundspeed == 8.25F);

    mavlink_distance_sensor_t distance{};
    distance.min_distance = 10U;
    distance.max_distance = 1000U;
    distance.current_distance = 444U;
    mavlink_msg_distance_sensor_encode(1U, 1U, &message, &distance);
    heartbeatHandler.processMessage(message);
    CHECK(distanceEvents == 1);
    CHECK(observedMinimumDistance == 20U);
    CHECK(observedMaximumDistance == 17500U);
    CHECK(observedCurrentDistance == 444U);

    mavlink_gimbal_device_attitude_status_t gimbalStatus{};
    gimbalStatus.time_boot_ms = 9876U;
    gimbalStatus.q[0] = 1.0F;
    gimbalStatus.angular_velocity_x = 1.0F;
    gimbalStatus.angular_velocity_y = 2.0F;
    gimbalStatus.angular_velocity_z = 3.0F;
    gimbalStatus.gimbal_device_id = 2U;
    mavlink_msg_gimbal_device_attitude_status_encode(
        1U, 172U, &message, &gimbalStatus);
    heartbeatHandler.processMessage(message);
    CHECK(gimbalEvents == 1);
    gimbalStatus.angular_velocity_x =
        std::numeric_limits<float>::quiet_NaN();
    mavlink_msg_gimbal_device_attitude_status_encode(
        1U, 172U, &message, &gimbalStatus);
    heartbeatHandler.processMessage(message);
    CHECK(gimbalEvents == 1);

    mavlink_param_value_t parameter{};
    parameter.param_value = 42.25F;
    std::memcpy(parameter.param_id, "TEST_PARAM", 10U);
    mavlink_msg_param_value_encode(1U, 1U, &message, &parameter);
    heartbeatHandler.processMessage(message);
    CHECK(parameterEvents == 1);
    CHECK(observedParameter == QStringLiteral("TEST_PARAM"));
    CHECK(observedParameterValue == 42.25F);

    mavlink_autopilot_version_t version{};
    std::memcpy(version.flight_custom_version, "45INAV06", 8U);
    mavlink_msg_autopilot_version_encode(1U, 1U, &message, &version);
    heartbeatHandler.processMessage(message);
    CHECK(validVersionEvents == 1);

    mavlink_battery_status_t battery{};
    battery.id = 3U;
    battery.current_battery = -123;
    mavlink_msg_battery_status_encode(1U, 1U, &message, &battery);
    heartbeatHandler.backupProcessMessage(
        QStringLiteral("backup"), message, 2U);
    CHECK(backupBatteryEvents == 1);
    CHECK(observedBatteryId == 3U);
    CHECK(observedBatteryCurrent == -123);

    qsizetype transmissionCount = link.transmissions.size();
    heartbeatHandler.readParam(QStringLiteral("SERVO3_MAX"));
    CHECK(link.transmissions.size() == transmissionCount + 1);
    mavlink_message_t generated =
        parseMavlink(link.transmissions.back(), MAVLINK_COMM_3);
    CHECK(generated.msgid == MAVLINK_MSG_ID_PARAM_REQUEST_READ);
    CHECK(generated.compid == MAV_COMP_ID_ONBOARD_COMPUTER);
    mavlink_param_request_read_t requestParameter{};
    mavlink_msg_param_request_read_decode(&generated, &requestParameter);
    CHECK(requestParameter.target_system == 51U);
    CHECK(requestParameter.target_component == 52U);
    CHECK(requestParameter.param_index == -1);

    transmissionCount = link.transmissions.size();
    heartbeatHandler.writeParam(QStringLiteral("TEST_WRITE"), 7.5F);
    CHECK(link.transmissions.size() == transmissionCount + 1);
    generated = parseMavlink(link.transmissions.back(), MAVLINK_COMM_3);
    CHECK(generated.msgid == MAVLINK_MSG_ID_PARAM_SET);
    CHECK(generated.compid == MAV_COMP_ID_ONBOARD_COMPUTER);
    mavlink_param_set_t setParameter{};
    mavlink_msg_param_set_decode(&generated, &setParameter);
    CHECK(setParameter.param_value == 7.5F);
    CHECK(setParameter.param_type == MAV_PARAM_TYPE_INT16);

    WindHandler windHandler(&communicator);
    int windEvents = 0;
    mavlink_wind_t observedWind{};
    QObject::connect(&windHandler,
                     &WindHandler::windEvent,
                     [&](const float direction,
                         const float speed,
                         const float speedZ) {
                         ++windEvents;
                         observedWind.direction = direction;
                         observedWind.speed = speed;
                         observedWind.speed_z = speedZ;
                     });
    mavlink_wind_t wind{123.5F, 17.25F, -2.5F};
    mavlink_msg_wind_encode(1U, 1U, &message, &wind);
    windHandler.processMessage(message);
    CHECK(windEvents == 1);
    CHECK(near(observedWind.direction, 123.5, 0.0));
    CHECK(near(observedWind.speed, 17.25, 0.0));
    CHECK(near(observedWind.speed_z, -2.5, 0.0));

    RcChannelsHandler rcHandler(&communicator);
    int homingOnEvents = 0;
    int homingOffEvents = 0;
    int backupHomingOnEvents = 0;
    int backupHomingOffEvents = 0;
    uint16_t observedRoll = 0;
    uint16_t observedPitch = 0;
    uint16_t observedThrottle = 0;
    uint16_t observedChannel18 = 0;
    QObject::connect(&rcHandler,
                     &RcChannelsHandler::homingOn,
                     [&] { ++homingOnEvents; });
    QObject::connect(&rcHandler,
                     &RcChannelsHandler::homingOff,
                     [&] { ++homingOffEvents; });
    QObject::connect(&rcHandler,
                     &RcChannelsHandler::backupHomingOn,
                     [&] { ++backupHomingOnEvents; });
    QObject::connect(&rcHandler,
                     &RcChannelsHandler::backupHomingOff,
                     [&] { ++backupHomingOffEvents; });
    QObject::connect(
        &rcHandler,
        &RcChannelsHandler::currentControls,
        [&](const uint16_t roll,
            const uint16_t pitch,
            const uint16_t throttle) {
            observedRoll = roll;
            observedPitch = pitch;
            observedThrottle = throttle;
        });
    QObject::connect(
        &rcHandler,
        &RcChannelsHandler::currentChannels,
        [&](uint16_t,
            uint16_t,
            uint16_t,
            uint16_t,
            uint16_t,
            uint16_t,
            uint16_t,
            uint16_t,
            uint16_t,
            uint16_t,
            uint16_t,
            uint16_t,
            uint16_t,
            const uint16_t channel18) { observedChannel18 = channel18; });
    mavlink_rc_channels_t channels{};
    channels.chan1_raw = 1100U;
    channels.chan2_raw = 1200U;
    channels.chan3_raw = 1300U;
    channels.chan5_raw = 1500U;
    channels.chan10_raw = 1700U;
    channels.chan18_raw = 1800U;
    mavlink_msg_rc_channels_encode(1U, 1U, &message, &channels);
    rcHandler.processMessage(message);
    CHECK(observedRoll == 1100U);
    CHECK(observedPitch == 1800U);
    CHECK(observedThrottle == 1300U);
    CHECK(observedChannel18 == 1800U);
    CHECK(homingOnEvents == 1 && homingOffEvents == 0);
    rcHandler.backupProcessMessage(QStringLiteral("backup"), message, 3U);
    CHECK(backupHomingOnEvents == 1 && backupHomingOffEvents == 0);
    channels.chan10_raw = 1100U;
    mavlink_msg_rc_channels_encode(1U, 1U, &message, &channels);
    rcHandler.processMessage(message);
    rcHandler.backupProcessMessage(QStringLiteral("backup"), message, 3U);
    CHECK(homingOffEvents == 1);
    CHECK(backupHomingOffEvents == 1);

    GpsHandler gpsHandler(&communicator);
    int rawPositionEvents = 0;
    int positionEvents = 0;
    int satelliteEvents = 0;
    uint32_t rawTime = 0;
    int32_t rawAltitude = 0;
    int32_t relativeAltitude = 0;
    QObject::connect(&gpsHandler,
                     &GpsHandler::satelliteCountChanged,
                     [&] { ++satelliteEvents; });
    QObject::connect(&gpsHandler,
                     &GpsHandler::rawPositionEvent,
                     [&](const uint32_t time,
                         int32_t,
                         int32_t,
                         const int32_t altitude) {
                         ++rawPositionEvents;
                         rawTime = time;
                         rawAltitude = altitude;
                     });
    QObject::connect(
        &gpsHandler,
        &GpsHandler::positionEvent,
        [&](uint32_t,
            int32_t,
            int32_t,
            int32_t,
            const int32_t mslAltitude,
            int16_t,
            int16_t,
            int16_t,
            uint16_t) {
            ++positionEvents;
            relativeAltitude = mslAltitude;
        });
    mavlink_gps_raw_int_t raw{};
    raw.time_usec = 12345678ULL;
    raw.lat = 515000000;
    raw.lon = 70000000;
    raw.alt = 123400;
    raw.satellites_visible = 12U;
    mavlink_msg_gps_raw_int_encode(1U, 1U, &message, &raw);
    gpsHandler.processMessage(message);
    gpsHandler.processMessage(message);
    CHECK(rawPositionEvents == 2);
    CHECK(rawTime == 12345U);
    CHECK(rawAltitude == 123400);
    CHECK(gpsHandler.satelliteCount() == 12U);
    CHECK(satelliteEvents == 1);
    raw.satellites_visible = 13U;
    mavlink_msg_gps_raw_int_encode(1U, 1U, &message, &raw);
    gpsHandler.processMessage(message);
    CHECK(satelliteEvents == 2);

    mavlink_global_position_int_t position{};
    position.time_boot_ms = 8765U;
    position.lat = raw.lat;
    position.lon = raw.lon;
    position.alt = 222000;
    position.relative_alt = 333000;
    position.vx = 10;
    position.vy = 20;
    position.vz = -30;
    position.hdg = 1234U;
    mavlink_msg_global_position_int_encode(1U, 1U, &message, &position);
    gpsHandler.processMessage(message);
    CHECK(positionEvents == 1);
    CHECK(relativeAltitude == 333000);

    AttitudeHandler attitudeHandler(&communicator, 20000);
    int attitudeEvents = 0;
    int backupAttitudeEvents = 0;
    int ahrsEvents = 0;
    float observedPitchAngle = 0.0F;
    float observedYaw = 0.0F;
    float observedRollAngle = 0.0F;
    QObject::connect(
        &attitudeHandler,
        &AttitudeHandler::attitudeEvent,
        [&](uint32_t,
            const float pitch,
            const float yaw,
            const float roll,
            float,
            float,
            float) {
            ++attitudeEvents;
            observedPitchAngle = pitch;
            observedYaw = yaw;
            observedRollAngle = roll;
        });
    QObject::connect(&attitudeHandler,
                     &AttitudeHandler::backupAttitudeEvent,
                     [&](unsigned char,
                         uint32_t,
                         float,
                         float,
                         float,
                         float,
                         float,
                         float) { ++backupAttitudeEvents; });
    QObject::connect(&attitudeHandler,
                     &AttitudeHandler::ahrsError,
                     [&](float, float) { ++ahrsEvents; });
    mavlink_attitude_t attitude{};
    attitude.time_boot_ms = 99U;
    attitude.roll = 1.0F;
    attitude.pitch = 2.0F;
    attitude.yaw = 3.0F;
    attitude.rollspeed = 4.0F;
    attitude.pitchspeed = 5.0F;
    attitude.yawspeed = 6.0F;
    mavlink_msg_attitude_encode(1U, 1U, &message, &attitude);
    attitudeHandler.processMessage(message);
    attitudeHandler.processMessage(message);
    attitudeHandler.backupProcessMessage(QStringLiteral("backup"), message, 4U);
    CHECK(attitudeEvents == 1);
    CHECK(backupAttitudeEvents == 1);
    CHECK(observedPitchAngle == 2.0F);
    CHECK(observedYaw == 3.0F);
    CHECK(observedRollAngle == 1.0F);

    mavlink_ahrs_t ahrs{};
    ahrs.error_rp = 0.125F;
    ahrs.error_yaw = -0.25F;
    mavlink_msg_ahrs_encode(1U, 1U, &message, &ahrs);
    attitudeHandler.processMessage(message);
    CHECK(ahrsEvents == 1);

    const qsizetype previousTransmissions = link.transmissions.size();
    attitudeHandler.setAHRS(123456789LL, 10.0F, 20.0F, 30.0F);
    CHECK(link.transmissions.size() == previousTransmissions + 1);
    generated =
        parseMavlink(link.transmissions.back(), MAVLINK_COMM_3);
    CHECK(generated.msgid == MAVLINK_MSG_ID_ATTITUDE);
    CHECK(generated.sysid == 51U);
    CHECK(generated.compid == MAV_COMP_ID_ONBOARD_COMPUTER);
    mavlink_attitude_t generatedAttitude{};
    mavlink_msg_attitude_decode(&generated, &generatedAttitude);
    CHECK(generatedAttitude.time_boot_ms == 123456789U);
    CHECK(near(generatedAttitude.roll, 0.174532925, 1.0e-7));
    CHECK(near(generatedAttitude.pitch, 0.349065850, 1.0e-7));
    CHECK(near(generatedAttitude.yaw, 0.523598776, 1.0e-7));

    TimeSyncHandler timeSyncHandler(&communicator);
    int timeOffsetEvents = 0;
    qlonglong observedOffset = 0;
    int observedRoundTrip = 0;
    QObject::connect(&timeSyncHandler,
                     &TimeSyncHandler::updateTimeOffset,
                     [&](const qlonglong offset, const int roundTrip) {
                         ++timeOffsetEvents;
                         observedOffset = offset;
                         observedRoundTrip = roundTrip;
                     });
    timeSyncHandler.startSync();
    for (int sample = 0; sample < 40; ++sample) {
        CHECK(QMetaObject::invokeMethod(&timeSyncHandler,
                                        "sendSyncMessage",
                                        Qt::DirectConnection));
        const mavlink_message_t request =
            parseMavlink(link.transmissions.back(), MAVLINK_COMM_3);
        CHECK(request.msgid == MAVLINK_MSG_ID_TIMESYNC);
        CHECK(request.sysid == 51U);
        CHECK(request.compid == MAV_COMP_ID_ONBOARD_COMPUTER);
        mavlink_timesync_t requestData{};
        mavlink_msg_timesync_decode(&request, &requestData);
        CHECK(requestData.tc1 == 0);
        CHECK(requestData.ts1 > 0);

        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        mavlink_timesync_t responseData{};
        responseData.ts1 = (now - 10) * 1000000LL;
        responseData.tc1 = responseData.ts1 - 105000000LL;
        mavlink_message_t response{};
        mavlink_msg_timesync_encode(1U, 1U, &response, &responseData);
        timeSyncHandler.processMessage(response);
    }
    CHECK(timeOffsetEvents == 1);
    CHECK(observedOffset >= 99 && observedOffset <= 100);
    CHECK(observedRoundTrip >= 10 && observedRoundTrip <= 11);
}

void testMavlinkControlHandler()
{
    MavLinkCommunicator communicator(51U, 52U);
    TestLink mainLink(QStringLiteral("main"));
    TestLink backupLink(QStringLiteral("backup"));
    communicator.addLink(&mainLink, MAVLINK_COMM_0, 1);
    communicator.addLink(&backupLink, MAVLINK_COMM_1, 2);

    ControlHandler handler(&communicator, 3600000);
    CHECK(handler.metaObject()->indexOfMethod("arm(bool)") >= 0);
    CHECK(handler.metaObject()->indexOfMethod("arm()") >= 0);
    CHECK(handler.metaObject()->indexOfMethod("controlOn()") >= 0);
    CHECK(handler.metaObject()->indexOfMethod(
              "setEPTZ(float,float,float,float,float,float,bool,bool,uchar)")
          >= 0);
    CHECK(handler.metaObject()->indexOfMethod(
              "mavCmdDoSet(uint16_t,float,float,float,float,float,float,float)")
          >= 0);
    CHECK(handler.currentMountControlCenterOnValue() == 1U);
    CHECK(handler.currentMountControlCenterOffValue() == 2U);

    handler.arm();
    CHECK(mainLink.transmissions.size() == 1);
    mavlink_message_t generated =
        parseMavlink(mainLink.transmissions.back(), MAVLINK_COMM_2);
    CHECK(generated.msgid == MAVLINK_MSG_ID_COMMAND_LONG);
    CHECK(generated.sysid == 1U);
    CHECK(generated.compid == MAV_COMP_ID_ONBOARD_COMPUTER);
    mavlink_command_long_t command{};
    mavlink_msg_command_long_decode(&generated, &command);
    CHECK(command.target_system == 1U);
    CHECK(command.target_component == 1U);
    CHECK(command.command == MAV_CMD_COMPONENT_ARM_DISARM);
    CHECK(command.param1 == 1.0F);
    CHECK(command.param2 == 2989.0F);

    handler.controlOn();
    generated = parseMavlink(mainLink.transmissions.back(), MAVLINK_COMM_2);
    mavlink_msg_command_long_decode(&generated, &command);
    CHECK(command.command == MAV_CMD_DO_SET_MODE);
    CHECK(command.param1 == 1.0F);
    CHECK(command.param2 == 5.0F);
    CHECK(command.param3 == 1.0F);

    handler.setBackupControllerEnabled(true);
    const qsizetype mainBeforeMode = mainLink.transmissions.size();
    const qsizetype backupBeforeMode = backupLink.transmissions.size();
    handler.setMode(9);
    CHECK(mainLink.transmissions.size() == mainBeforeMode + 1);
    CHECK(backupLink.transmissions.size() == backupBeforeMode + 1);

    handler.setControlParams(500, 2500, 1200, 1000);
    handler.setThrottleMinValue(900U);
    handler.enableRpmControl(true);
    handler.startRpmControl();
    handler.setParachuteEnabled(true);
    handler.parachuteOn();
    handler.setReverseThrottleEnabled(true);
    handler.setReverseThrottle();
    handler.setFlapsEnabled(true);
    handler.setFlapsValue(1777);
    handler.setMinelayerEnabled(true);
    handler.dropMinelayer();
    handler.enableSpyCamera(true);
    handler.openSpyCamera();
    handler.setPTZCameraEnabled(true);
    handler.ptzCameraOn();
    handler.setBackupSystemId(77);
    handler.setBackupControllerInvertPitch(true);
    handler.setBackupControllerDropEnabled(true);
    handler.startBackupControllerDrop();

    const qsizetype mainBeforeOverride = mainLink.transmissions.size();
    const qsizetype backupBeforeOverride = backupLink.transmissions.size();
    CHECK(QMetaObject::invokeMethod(
        &handler, "sendChanValues", Qt::DirectConnection));
    CHECK(mainLink.transmissions.size() == mainBeforeOverride + 1);
    CHECK(backupLink.transmissions.size() == backupBeforeOverride + 1);

    generated = parseMavlink(mainLink.transmissions.back(), MAVLINK_COMM_2);
    CHECK(generated.msgid == MAVLINK_MSG_ID_RC_CHANNELS_OVERRIDE);
    CHECK(generated.sysid == 255U);
    CHECK(generated.compid == MAV_COMP_ID_ONBOARD_COMPUTER);
    mavlink_rc_channels_override_t primary{};
    mavlink_msg_rc_channels_override_decode(&generated, &primary);
    CHECK(primary.target_system == 51U);
    CHECK(primary.target_component == 52U);
    CHECK(primary.chan1_raw == 1000U);
    CHECK(primary.chan2_raw == 1800U);
    CHECK(primary.chan3_raw == 900U);
    CHECK(primary.chan4_raw == 2000U);
    CHECK(primary.chan5_raw == 0U);
    CHECK(primary.chan6_raw == 1777U);
    CHECK(primary.chan7_raw == 1000U);
    CHECK(primary.chan12_raw == 2000U);

    generated = parseMavlink(backupLink.transmissions.back(), MAVLINK_COMM_2);
    CHECK(generated.msgid == MAVLINK_MSG_ID_RC_CHANNELS_OVERRIDE);
    CHECK(generated.sysid == 77U);
    CHECK(generated.compid == MAV_COMP_ID_ONBOARD_COMPUTER);
    mavlink_rc_channels_override_t backup{};
    mavlink_msg_rc_channels_override_decode(&generated, &backup);
    CHECK(backup.target_system == 51U);
    CHECK(backup.target_component == 52U);
    CHECK(backup.chan1_raw == 1000U);
    CHECK(backup.chan2_raw == 1800U);
    CHECK(backup.chan3_raw == 1000U);
    CHECK(backup.chan4_raw == 2000U);
    CHECK(backup.chan12_raw == 2000U);

    handler.controlOff();
    generated = parseMavlink(mainLink.transmissions.back(), MAVLINK_COMM_2);
    mavlink_msg_rc_channels_override_decode(&generated, &primary);
    CHECK(primary.chan1_raw == 0U);
    CHECK(primary.chan2_raw == 0U);
    CHECK(primary.chan3_raw == 0U);
    CHECK(primary.chan4_raw == 0U);
    CHECK(primary.chan6_raw == 0U);
    CHECK(primary.chan7_raw == 1000U);
    CHECK(primary.chan12_raw == 0U);
    generated = parseMavlink(backupLink.transmissions.back(), MAVLINK_COMM_2);
    mavlink_msg_rc_channels_override_decode(&generated, &backup);
    CHECK(backup.chan1_raw == 0U);
    CHECK(backup.chan12_raw == 0U);

    const qsizetype beforeParachuteEnable = mainLink.transmissions.size();
    handler.setParachuteEnabled(true);
    CHECK(mainLink.transmissions.size() == beforeParachuteEnable + 1);
    generated = parseMavlink(mainLink.transmissions.back(), MAVLINK_COMM_2);
    mavlink_msg_rc_channels_override_decode(&generated, &primary);
    CHECK(primary.chan6_raw == 1000U);

    handler.setMountControlCenterOnValue(9U);
    handler.setMountControlCenterOffValue(8U);
    handler.setEPTZ(10.0F,
                    20.0F,
                    30.0F,
                    40.0F,
                    50.0F,
                    60.0F,
                    false,
                    true,
                    154U);
    generated = parseMavlink(mainLink.transmissions.back(), MAVLINK_COMM_2);
    mavlink_msg_command_long_decode(&generated, &command);
    CHECK(command.command == MAV_CMD_DO_MOUNT_CONTROL);
    CHECK(command.target_system == 1U);
    CHECK(command.target_component == 154U);
    CHECK(command.param1 == 20.0F);
    CHECK(command.param2 == 10.0F);
    CHECK(command.param3 == 30.0F);
    CHECK(command.param4 == 0.0F);
    CHECK(command.param5 == 0.0F);
    CHECK(command.param6 == 0.0F);
    CHECK(command.param7 == 8.0F);

    handler.setEPTZ(10.0F,
                    20.0F,
                    30.0F,
                    40.0F,
                    50.0F,
                    60.0F,
                    false,
                    true,
                    172U);
    generated = parseMavlink(mainLink.transmissions.back(), MAVLINK_COMM_2);
    mavlink_msg_command_long_decode(&generated, &command);
    CHECK(command.target_component == 172U);
    CHECK(command.param1 == 30.0F);
    CHECK(command.param2 == 10.0F);
    CHECK(command.param3 == 20.0F);
    CHECK(command.param4 == 60.0F);
    CHECK(command.param5 == 40.0F);
    CHECK(command.param6 == 50.0F);
    CHECK(command.param7 == 2.0F);

    handler.setEPTZ(10.0F,
                    20.0F,
                    30.0F,
                    40.0F,
                    50.0F,
                    60.0F,
                    true,
                    false,
                    173U);
    generated = parseMavlink(mainLink.transmissions.back(), MAVLINK_COMM_2);
    mavlink_msg_command_long_decode(&generated, &command);
    CHECK(command.target_component == 173U);
    CHECK(command.param7 == 3.0F);

    handler.writeParam(QStringLiteral("CTRL_PARAM"), -123);
    generated = parseMavlink(mainLink.transmissions.back(), MAVLINK_COMM_2);
    CHECK(generated.msgid == MAVLINK_MSG_ID_PARAM_SET);
    CHECK(generated.sysid == 51U);
    CHECK(generated.compid == MAV_COMP_ID_ONBOARD_COMPUTER);
    mavlink_param_set_t parameter{};
    mavlink_msg_param_set_decode(&generated, &parameter);
    CHECK(parameter.target_system == 51U);
    CHECK(parameter.target_component == 52U);
    CHECK(parameter.param_value == -123.0F);
    CHECK(parameter.param_type == MAV_PARAM_TYPE_INT16);
    CHECK(std::memcmp(parameter.param_id, "CTRL_PARAM", 10U) == 0);

    int startAckEvents = 0;
    int acceptAckEvents = 0;
    int cancelAckEvents = 0;
    int levelAckEvents = 0;
    int progressEvents = 0;
    int statusEvents = 0;
    int rpmEvents = 0;
    unsigned char observedResult = 0U;
    unsigned char observedProgress = 0U;
    unsigned char observedStatus = 0U;
    int observedRpm1 = 0;
    int observedRpm2 = 0;
    QObject::connect(&handler,
                     &ControlHandler::startMagCalAck,
                     [&](const unsigned char result) {
                         ++startAckEvents;
                         observedResult = result;
                     });
    QObject::connect(&handler,
                     &ControlHandler::acceptMagCalAck,
                     [&](unsigned char) { ++acceptAckEvents; });
    QObject::connect(&handler,
                     &ControlHandler::cancelMagCalAck,
                     [&](unsigned char) { ++cancelAckEvents; });
    QObject::connect(&handler,
                     &ControlHandler::levelCalAck,
                     [&](unsigned char) { ++levelAckEvents; });
    QObject::connect(
        &handler,
        &ControlHandler::magCalCompletionPercentage,
        [&](const unsigned char value) {
            ++progressEvents;
            observedProgress = value;
        });
    QObject::connect(&handler,
                     &ControlHandler::magCalStatus,
                     [&](const unsigned char value) {
                         ++statusEvents;
                         observedStatus = value;
                     });
    QObject::connect(&handler,
                     &ControlHandler::rpmValues,
                     [&](const int rpm1, const int rpm2) {
                         ++rpmEvents;
                         observedRpm1 = rpm1;
                         observedRpm2 = rpm2;
                     });

    mavlink_message_t incoming{};
    mavlink_command_ack_t ack{};
    ack.command = 42424U;
    ack.result = MAV_RESULT_TEMPORARILY_REJECTED;
    mavlink_msg_command_ack_encode(1U, 1U, &incoming, &ack);
    handler.processMessage(incoming);
    CHECK(startAckEvents == 1);
    CHECK(observedResult == MAV_RESULT_TEMPORARILY_REJECTED);
    ack.command = 42425U;
    mavlink_msg_command_ack_encode(1U, 1U, &incoming, &ack);
    handler.processMessage(incoming);
    ack.command = 42426U;
    mavlink_msg_command_ack_encode(1U, 1U, &incoming, &ack);
    handler.processMessage(incoming);
    ack.command = MAV_CMD_PREFLIGHT_CALIBRATION;
    mavlink_msg_command_ack_encode(1U, 1U, &incoming, &ack);
    handler.processMessage(incoming);
    CHECK(acceptAckEvents == 1);
    CHECK(cancelAckEvents == 1);
    CHECK(levelAckEvents == 1);

    mavlink_mag_cal_progress_t progress{};
    progress.completion_pct = 25U;
    mavlink_msg_mag_cal_progress_encode(1U, 1U, &incoming, &progress);
    handler.processMessage(incoming);
    CHECK(progressEvents == 1);
    CHECK(observedProgress == 25U);
    progress.completion_pct = 100U;
    mavlink_msg_mag_cal_progress_encode(1U, 1U, &incoming, &progress);
    handler.processMessage(incoming);
    CHECK(progressEvents == 2);
    CHECK(observedProgress == 100U);

    mavlink_mag_cal_report_t report{};
    report.cal_status = MAG_CAL_SUCCESS;
    mavlink_msg_mag_cal_report_encode(1U, 1U, &incoming, &report);
    handler.processMessage(incoming);
    CHECK(statusEvents == 1);
    CHECK(observedStatus == MAG_CAL_SUCCESS);

    mavlink_rpm_t rpm{};
    rpm.rpm1 = 1234.75F;
    rpm.rpm2 = -98.5F;
    mavlink_msg_rpm_encode(1U, 1U, &incoming, &rpm);
    handler.processMessage(incoming);
    CHECK(rpmEvents == 1);
    CHECK(observedRpm1 == 1234);
    CHECK(observedRpm2 == -98);
}

void testMavlinkLogHandler()
{
    const QString previousDirectory = QDir::currentPath();
    QTemporaryDir temporaryDirectory;
    CHECK(temporaryDirectory.isValid());
    if (!temporaryDirectory.isValid()) {
        return;
    }
    CHECK(QDir::setCurrent(temporaryDirectory.path()));

    MavLinkCommunicator communicator(51U, 52U);
    TestLink link(QStringLiteral("telemetry"));
    communicator.addLink(&link, MAVLINK_COMM_3, 9);
    LogHandler handler(&communicator);

    CHECK(handler.metaObject()->indexOfMethod(
              "logListEvent(QList<LogItem>)")
          >= 0);
    CHECK(handler.metaObject()->indexOfMethod("getLogError(int)") >= 0);
    CHECK(!handler.isRunning());
    CHECK(handler.downloaded() == 0U);
    CHECK(handler.remaining() == 0U);
    CHECK(handler.loadingId() == 0U);

    QList<LogItem> observedItems;
    int listEvents = 0;
    int listErrors = 0;
    int logErrors = 0;
    int logEvents = 0;
    QByteArray observedLog;
    QObject::connect(&handler,
                     &LogHandler::logListEvent,
                     [&](const QList<LogItem>& items) {
                         ++listEvents;
                         observedItems = items;
                     });
    QObject::connect(&handler,
                     &LogHandler::getLogListError,
                     [&](const int code) {
                         CHECK(code == 1);
                         ++listErrors;
                     });
    QObject::connect(&handler,
                     &LogHandler::getLogError,
                     [&](const int code) {
                         CHECK(code == 2);
                         ++logErrors;
                     });
    QObject::connect(&handler,
                     &LogHandler::logEvent,
                     [&](const QByteArray& data) {
                         ++logEvents;
                         observedLog = data;
                     });

    CHECK(QDir().mkdir(QStringLiteral("mavlink_logs")));
    QFile cached(QStringLiteral("mavlink_logs/3_3000_3"));
    CHECK(cached.open(QIODevice::WriteOnly));
    CHECK(cached.write("XYZ", 3) == 3);
    cached.close();

    qsizetype transmissionCount = link.transmissions.size();
    CHECK(handler.getLogList());
    CHECK(handler.isRunning());
    CHECK(link.transmissions.size() == transmissionCount + 1);
    CHECK(!handler.getLogList());
    CHECK(link.transmissions.size() == transmissionCount + 1);

    mavlink_message_t generated =
        parseMavlink(link.transmissions.back(), MAVLINK_COMM_3);
    CHECK(generated.msgid == MAVLINK_MSG_ID_LOG_REQUEST_LIST);
    CHECK(generated.sysid == 51U);
    CHECK(generated.compid == MAV_COMP_ID_ONBOARD_COMPUTER);
    mavlink_log_request_list_t listRequest{};
    mavlink_msg_log_request_list_decode(&generated, &listRequest);
    CHECK(listRequest.target_system == 51U);
    CHECK(listRequest.target_component == 52U);
    CHECK(listRequest.start == 0U);
    CHECK(listRequest.end == UINT16_MAX);

    mavlink_message_t message{};
    mavlink_msg_log_entry_pack(
        1U, 1U, &message, 1U, 3U, 3U, 1000U, 4U);
    handler.processMessage(message);
    CHECK(listEvents == 0);
    CHECK(handler.isRunning());
    mavlink_msg_log_entry_pack(
        1U, 1U, &message, 2U, 3U, 3U, 2000U, 5U);
    handler.processMessage(message);
    CHECK(listEvents == 0);
    mavlink_msg_log_entry_pack(
        1U, 1U, &message, 3U, 3U, 3U, 3000U, 3U);
    handler.processMessage(message);
    CHECK(listEvents == 1);
    CHECK(!handler.isRunning());
    CHECK(observedItems.size() == 3);
    CHECK(observedItems[0].id == 1U);
    CHECK(observedItems[0].time == 1000U);
    CHECK(observedItems[0].size == 4U);
    CHECK(!observedItems[0].loaded);
    CHECK(!observedItems[1].loaded);
    CHECK(observedItems[2].loaded);

    transmissionCount = link.transmissions.size();
    CHECK(handler.getLog(3));
    CHECK(logEvents == 1);
    CHECK(observedLog == QByteArray("XYZ", 3));
    CHECK(link.transmissions.size() == transmissionCount);

    CHECK(handler.getLog(1));
    CHECK(handler.isRunning());
    CHECK(handler.loadingId() == 1U);
    CHECK(handler.downloaded() == 0U);
    CHECK(handler.remaining() == 4U);
    CHECK(link.transmissions.size() == transmissionCount + 1);
    generated = parseMavlink(link.transmissions.back(), MAVLINK_COMM_3);
    CHECK(generated.msgid == MAVLINK_MSG_ID_LOG_REQUEST_DATA);
    CHECK(generated.compid == MAV_COMP_ID_ONBOARD_COMPUTER);
    mavlink_log_request_data_t dataRequest{};
    mavlink_msg_log_request_data_decode(&generated, &dataRequest);
    CHECK(dataRequest.target_system == 51U);
    CHECK(dataRequest.target_component == 52U);
    CHECK(dataRequest.id == 1U);
    CHECK(dataRequest.ofs == 0U);
    CHECK(dataRequest.count == 4U);

    std::array<std::uint8_t, 90> payload{};
    std::memcpy(payload.data(), "DATA", 4U);
    mavlink_msg_log_data_pack(
        1U, 1U, &message, 1U, 1U, 4U, payload.data());
    handler.processMessage(message);
    CHECK(logErrors == 1);
    CHECK(!handler.isRunning());

    CHECK(handler.getLog(1));
    mavlink_msg_log_data_pack(
        1U, 1U, &message, 1U, 0U, 4U, payload.data());
    handler.processMessage(message);
    CHECK(logEvents == 2);
    CHECK(observedLog == QByteArray("DATA", 4));
    CHECK(!handler.isRunning());
    CHECK(handler.downloaded() == 0U);
    CHECK(handler.remaining() == 0U);

    QFile downloaded(QStringLiteral("mavlink_logs/1_1000_4"));
    CHECK(downloaded.open(QIODevice::ReadOnly));
    CHECK(downloaded.readAll() == QByteArray("DATA", 4));
    downloaded.close();

    // RVA 0x179c54 marks the item at the one-based loading identifier.
    // Downloading identifier 1 therefore marks list element 1 (the second
    // item), so this cache read emits an empty payload without a request.
    transmissionCount = link.transmissions.size();
    CHECK(handler.getLog(2));
    CHECK(logEvents == 3);
    CHECK(observedLog.isEmpty());
    CHECK(link.transmissions.size() == transmissionCount);

    CHECK(handler.getLogList());
    mavlink_msg_log_entry_pack(
        1U, 1U, &message, 2U, 1U, 2U, 2000U, 5U);
    handler.processMessage(message);
    CHECK(listErrors == 1);
    CHECK(!handler.isRunning());

    CHECK(QDir::setCurrent(previousDirectory));
}
#endif

void testELinkCore()
{
    ELinkCommunicator communicator;
    CHECK(communicator.messageDelay() == 50U);
    CHECK(communicator.metaObject()->indexOfSignal(
              "messageReceived(QByteArray)")
          >= 0);
    CHECK(communicator.metaObject()->indexOfSlot("addLink(AbstractLink*)")
          >= 0);
    CHECK(communicator.metaObject()->indexOfSlot(
              "sendMessageOnAllLinks(QByteArray)")
          >= 0);

    TestELink first;
    TestELink second;
    communicator.addLink(&first);
    communicator.addLink(&first);
    communicator.addLink(&second);
    CHECK(communicator.links().size() == 2);

    communicator.sendMessageOnAllLinks(QByteArray("all", 3));
    CHECK(first.transmissions.size() == 1);
    CHECK(second.transmissions.size() == 1);
    CHECK(first.transmissions.back() == QByteArray("all", 3));
    communicator.sendMessage(QByteArray("one", 3), &second);
    CHECK(first.transmissions.size() == 1);
    CHECK(second.transmissions.size() == 2);
    CHECK(second.transmissions.back() == QByteArray("one", 3));

    communicator.addMessageTimeDelay(25);
    CHECK(communicator.messageDelay() == 75U);

    TestELinkHandler handler(&communicator);
    CHECK(handler.metaObject()->indexOfSlot("processMessage(QByteArray)")
          >= 0);
    int messageEvents = 0;
    QByteArray observedMessage;
    QObject::connect(&communicator,
                     &ELinkCommunicator::messageReceived,
                     [&](const QByteArray& message) {
                         ++messageEvents;
                         observedMessage = message;
                     });

    QByteArray payload;
    payload.append(static_cast<char>(0x42));
    payload.append(static_cast<char>(0x99));
    appendTestLittleEndian(payload, static_cast<std::uint8_t>(0xfe));
    appendTestLittleEndian(payload, static_cast<std::int16_t>(-1234));
    appendTestLittleEndian(payload, static_cast<std::uint16_t>(60000));
    appendTestLittleEndian(payload, static_cast<std::uint32_t>(0x89abcdef));
    appendTestLittleEndian(payload, -1234567);
    appendTestLittleEndian(payload, 12.5F);
    CHECK(payload.size() == 19);

    QByteArray frame = QByteArray::fromHex("d00d");
    appendTestLittleEndian(frame,
                           static_cast<std::uint16_t>(payload.size()));
    frame.append(payload);

    first.feed(QByteArray::fromHex("0011d0"));
    first.feed(frame.mid(1, 4));
    CHECK(messageEvents == 0);
    first.feed(frame.mid(5));
    CHECK(messageEvents == 1);
    CHECK(observedMessage == payload);
    CHECK(handler.processCount == 1);
    CHECK(handler.messageType() == 0x99U);
    CHECK(handler.messageSize() == 19U);
    CHECK(handler.uint8Value == 0xfeU);
    CHECK(handler.int16Value == -1234);
    CHECK(handler.uint16Value == 60000U);
    CHECK(handler.uint32Value == 0x89abcdefU);
    CHECK(handler.intValue == -1234567);
    CHECK(near(handler.floatValue, 12.5, 0.0));
    CHECK(std::abs(QDateTime::currentMSecsSinceEpoch()
                   - communicator.messageDelay() - handler.messageTime())
          < 1000);

    QByteArray twoFrames = frame;
    twoFrames.append(frame);
    first.feed(twoFrames);
    CHECK(messageEvents == 3);
    CHECK(handler.processCount == 3);

    first.feed(QByteArray::fromHex("d00d0000"));
    CHECK(messageEvents == 3);

    communicator.removeLink(&first);
    CHECK(communicator.links().size() == 1);
    first.feed(frame);
    CHECK(messageEvents == 3);
    communicator.removeLink(&first);
    CHECK(communicator.links().size() == 1);
}

void testELinkFoundationalHandlers()
{
    ELinkCommunicator communicator;
    TestELink link;
    communicator.addLink(&link);

    ELinkArmHandler arm(&communicator);
    CHECK(arm.metaObject()->indexOfSignal("armTestOnEvent()") >= 0);
    CHECK(arm.metaObject()->indexOfSignal("armTestOffEvent()") >= 0);
    CHECK(arm.metaObject()->indexOfSlot("processMessage()") >= 0);
    CHECK(arm.metaObject()->indexOfSlot("armOn()") >= 0);
    CHECK(arm.metaObject()->indexOfSlot("selfDestruction()") >= 0);
    int armTestEvents = 0;
    QObject::connect(&arm,
                     &ELinkArmHandler::armTestOnEvent,
                     [&] { ++armTestEvents; });
    QObject::connect(&arm,
                     &ELinkArmHandler::armTestOffEvent,
                     [&] { ++armTestEvents; });
    link.feed(makeTestELinkFrame(0x90U, QByteArray(1, '\1')));
    CHECK(armTestEvents == 0);
    CHECK(link.transmissions.size() == 1);
    checkTestELinkFrame(link.transmissions.back(),
                        0U,
                        0x90U,
                        QByteArray(1, static_cast<char>(0x5a)));
    link.feed(makeTestELinkFrame(0x90U, QByteArray(1, '\0')));
    CHECK(link.transmissions.size() == 1);

    arm.armOn();
    CHECK(link.transmissions.size() == 2);
    checkTestELinkFrame(link.transmissions.back(),
                        1U,
                        0x91U,
                        QByteArray(1, static_cast<char>(0x6b)));

    ELinkAttitudeHandler attitude(&communicator);
    CHECK(attitude.metaObject()->indexOfSignal(
              "attitudeEvent(qlonglong,float,float,float)")
          >= 0);
    CHECK(attitude.metaObject()->indexOfSignal("mgCfgStatus(uchar)") >= 0);
    CHECK(attitude.metaObject()->indexOfSignal("mgInited()") >= 0);
    CHECK(attitude.metaObject()->indexOfSignal("mgReseted(uchar)") >= 0);
    int attitudeEvents = 0;
    int configurationEvents = 0;
    int initializedEvents = 0;
    int resetEvents = 0;
    unsigned char lastStatus = 0U;
    unsigned char lastResetResult = 0U;
    QObject::connect(&attitude,
                     &ELinkAttitudeHandler::attitudeEvent,
                     [&](qlonglong, float, float, float) {
                         ++attitudeEvents;
                     });
    QObject::connect(&attitude,
                     &ELinkAttitudeHandler::mgCfgStatus,
                     [&](const unsigned char status) {
                         ++configurationEvents;
                         lastStatus = status;
                     });
    QObject::connect(&attitude,
                     &ELinkAttitudeHandler::mgInited,
                     [&] { ++initializedEvents; });
    QObject::connect(&attitude,
                     &ELinkAttitudeHandler::mgReseted,
                     [&](const unsigned char result) {
                         ++resetEvents;
                         lastResetResult = result;
                     });

    QByteArray orientation;
    appendTestLittleEndian(orientation, 0.25F);
    appendTestLittleEndian(orientation, -0.5F);
    appendTestLittleEndian(orientation, 7.0F);
    link.feed(makeTestELinkFrame(0x60U, orientation));
    CHECK(attitudeEvents == 0);

    QByteArray configuration;
    appendTestLittleEndian(configuration, std::uint32_t{1234U});
    configuration.append(static_cast<char>(0x0b));
    appendTestLittleEndian(configuration, std::uint32_t{5678U});
    link.feed(makeTestELinkFrame(0x0fU, configuration));
    link.feed(makeTestELinkFrame(0x0fU, configuration));
    CHECK(configurationEvents == 2);
    CHECK(lastStatus == 0x0bU);
    CHECK(initializedEvents == 1);

    link.feed(makeTestELinkFrame(
        0x65U, QByteArray(1, static_cast<char>(0xa5))));
    CHECK(resetEvents == 1);
    CHECK(lastResetResult == 0xa5U);
    attitude.mgReset();
    CHECK(link.transmissions.size() == 3);
    checkTestELinkFrame(link.transmissions.back(),
                        0U,
                        0x65U,
                        QByteArray(1, '\0'));

    ELinkMovemenetHandler movement(&communicator);
    CHECK(movement.metaObject()->indexOfSignal(
              "accelerationEvent(qlonglong,uint,uint,uint,int)")
          >= 0);
    CHECK(movement.metaObject()->indexOfSignal(
              "airspeedRawEvent(qlonglong,int,int)")
          >= 0);
    CHECK(movement.metaObject()->indexOfSignal(
              "airspeedEvent(qlonglong,float)")
          >= 0);
    int accelerationEvents = 0;
    int airspeedRawEvents = 0;
    int airspeedEvents = 0;
    qlonglong accelerationTime = 0;
    unsigned int observedXacc = 0U;
    unsigned int observedYacc = 0U;
    unsigned int observedZacc = 0U;
    int observedAccelerationTemperature = 0;
    int observedPressure = 0;
    int observedAirspeedTemperature = 0;
    float observedAirspeed = 0.0F;
    QObject::connect(
        &movement,
        &ELinkMovemenetHandler::accelerationEvent,
        [&](const qlonglong time,
            const unsigned int xacc,
            const unsigned int yacc,
            const unsigned int zacc,
            const int temperature) {
            ++accelerationEvents;
            accelerationTime = time;
            observedXacc = xacc;
            observedYacc = yacc;
            observedZacc = zacc;
            observedAccelerationTemperature = temperature;
        });
    QObject::connect(
        &movement,
        &ELinkMovemenetHandler::airspeedRawEvent,
        [&](qlonglong, const int pressure, const int temperature) {
            ++airspeedRawEvents;
            observedPressure = pressure;
            observedAirspeedTemperature = temperature;
        });
    QObject::connect(&movement,
                     &ELinkMovemenetHandler::airspeedEvent,
                     [&](qlonglong, const float speed) {
                         ++airspeedEvents;
                         observedAirspeed = speed;
                     });

    QByteArray acceleration;
    appendTestLittleEndian(acceleration, -1);
    appendTestLittleEndian(acceleration, 2);
    appendTestLittleEndian(acceleration, 3);
    appendTestLittleEndian(acceleration, 4);
    link.feed(makeTestELinkFrame(0x10U, acceleration));
    CHECK(accelerationEvents == 1);
    CHECK(observedXacc == std::numeric_limits<unsigned int>::max());
    CHECK(observedYacc == 2U);
    CHECK(observedZacc == 3U);
    CHECK(observedAccelerationTemperature == 3);
    CHECK(std::abs(QDateTime::currentMSecsSinceEpoch() - 50
                   - accelerationTime)
          < 1000);

    QByteArray rawAirspeed;
    appendTestLittleEndian(rawAirspeed, -101325);
    appendTestLittleEndian(rawAirspeed, 2715);
    link.feed(makeTestELinkFrame(0x20U, rawAirspeed));
    CHECK(airspeedRawEvents == 1);
    CHECK(observedPressure == -101325);
    CHECK(observedAirspeedTemperature == 2715);

    QByteArray airspeed;
    appendTestLittleEndian(airspeed, 12.75F);
    link.feed(makeTestELinkFrame(0x21U, airspeed));
    CHECK(airspeedEvents == 1);
    CHECK(observedAirspeed == 12.75F);
}

void testELinkStatusAndInterceptorHandlers()
{
    ELinkCommunicator communicator;
    TestELink link;
    communicator.addLink(&link);

    ELinkInterceptorHandler interceptor(&communicator);
    CHECK(interceptor.metaObject()->indexOfSignal("targetDetected()") >= 0);
    CHECK(interceptor.metaObject()->indexOfSlot("processMessage()") >= 0);
    int targetEvents = 0;
    QObject::connect(&interceptor,
                     &ELinkInterceptorHandler::targetDetected,
                     [&] { ++targetEvents; });
    link.feed(makeTestELinkFrame(
        0x9aU, QByteArray::fromHex("00000001")));
    CHECK(targetEvents == 1);
    link.feed(makeTestELinkFrame(
        0x9aU, QByteArray::fromHex("00000001")));
    CHECK(targetEvents == 1);

    ELinkStatusHandler status(&communicator);
    CHECK(status.metaObject()->indexOfSignal(
              "batteryVoltageChanged(uint)")
          >= 0);
    CHECK(status.metaObject()->indexOfSignal("buttonStateChanged(bool)")
          >= 0);
    CHECK(status.metaObject()->indexOfSlot("setLedState(LedState)") >= 0);
    CHECK(status.metaObject()->indexOfSlot("getBoardVersion()") >= 0);
    CHECK(led2string(LedState::None) == QStringLiteral("None"));
    CHECK(led2string(LedState::Ready) == QStringLiteral("Ready"));
    CHECK(led2string(LedState::NotReady) == QStringLiteral("NotReady"));
    CHECK(led2string(LedState::Armed) == QStringLiteral("Armed"));
    CHECK(led2string(static_cast<LedState>(99))
          == QStringLiteral("UNKNOWN"));

    int batteryEvents = 0;
    int buttonEvents = 0;
    unsigned int observedVoltage = 0U;
    bool observedButton = false;
    QObject::connect(&status,
                     &ELinkStatusHandler::batteryVoltageChanged,
                     [&](const unsigned int voltage) {
                         ++batteryEvents;
                         observedVoltage = voltage;
                     });
    QObject::connect(&status,
                     &ELinkStatusHandler::buttonStateChanged,
                     [&](const bool pressed) {
                         ++buttonEvents;
                         observedButton = pressed;
                     });

    QByteArray voltage;
    appendTestLittleEndian(voltage, std::uint32_t{24500U});
    link.feed(makeTestELinkFrame(0x80U, voltage));
    link.feed(makeTestELinkFrame(0x80U, voltage));
    CHECK(batteryEvents == 1);
    CHECK(observedVoltage == 24500U);

    link.feed(makeTestELinkFrame(0xb1U, QByteArray::fromHex("ff01")));
    CHECK(buttonEvents == 1);
    CHECK(observedButton);
    link.feed(makeTestELinkFrame(0xb1U, QByteArray::fromHex("ff00")));
    CHECK(buttonEvents == 2);
    CHECK(!observedButton);

    status.getBoardVersion();
    CHECK(link.transmissions.size() == 1);
    checkTestELinkFrame(link.transmissions.back(),
                        0U,
                        0x02U,
                        QByteArray(1, '\0'));

    QByteArray version;
    appendTestLittleEndian(version, std::uint32_t{1U});
    appendTestLittleEndian(version, std::uint32_t{2U});
    appendTestLittleEndian(version, std::uint32_t{3U});
    appendTestLittleEndian(version, std::uint16_t{0x1234U});
    appendTestLittleEndian(version, std::uint16_t{5U});
    link.feed(makeTestELinkFrame(0x02U, version));
    status.getBoardVersion();
    CHECK(link.transmissions.size() == 1);

    link.feed(makeTestELinkFrame(
        0x04U, QByteArray(1, static_cast<char>(0x7e))));
    CHECK(link.transmissions.size() == 2);
    checkTestELinkFrame(link.transmissions.back(),
                        1U,
                        0x04U,
                        QByteArray(1, static_cast<char>(0x7e)));

    status.setLedState(LedState::None);
    status.setLedState(LedState::Ready);
    status.setLedState(LedState::NotReady);
    status.setLedState(LedState::Armed);
    CHECK(link.transmissions.size() == 6);
    checkTestELinkFrame(link.transmissions[2],
                        2U,
                        0xb0U,
                        QByteArray::fromHex("0000"));
    checkTestELinkFrame(link.transmissions[3],
                        3U,
                        0xb0U,
                        QByteArray::fromHex("0100"));
    checkTestELinkFrame(link.transmissions[4],
                        4U,
                        0xb0U,
                        QByteArray::fromHex("0200"));
    checkTestELinkFrame(link.transmissions[5],
                        5U,
                        0xb0U,
                        QByteArray::fromHex("0103"));
}

void testELinkPositionHandler()
{
    ELinkCommunicator communicator;
    TestELink link;
    communicator.addLink(&link);
    ELinkPositionHandler handler(&communicator);

    CHECK(handler.metaObject()->indexOfSignal(
              "positionEvent(qlonglong,uchar,uchar,int,int,int,float,float,float,float,float)")
          >= 0);
    CHECK(handler.metaObject()->indexOfSignal(
              "backupPositionEvent(qlonglong,uchar,uchar,int,int,int,float,float,float,float,float)")
          >= 0);
    CHECK(handler.metaObject()->indexOfSignal(
              "distanceSensorEvent(uint,uint,uint)")
          >= 0);
    CHECK(handler.metaObject()->indexOfSlot(
              "setManualAccelerationEvent()")
          >= 0);
    CHECK(handler.metaObject()->indexOfSlot("sendAirspeed(float)") >= 0);

    int barometricRawEvents = 0;
    int magneticEvents = 0;
    int positionEvents = 0;
    int backupPositionEvents = 0;
    int distanceEvents = 0;
    int barometricAltitudeEvents = 0;
    int launchEvents = 0;
    int observedPressure = 0;
    int observedTemperature = 0;
    int observedMagnetic[3]{};
    unsigned char observedStatus = 0U;
    unsigned char observedSatellites = 0U;
    int observedLatitude = 0;
    int observedLongitude = 0;
    int observedAltitude = 0;
    float observedVelocityX = 0.0F;
    float observedVelocityY = 0.0F;
    float observedVelocityZ = 0.0F;
    float observedGroundSpeed = 0.0F;
    float observedGroundCourse = 0.0F;
    unsigned int observedMinimumDistance = 0U;
    unsigned int observedMaximumDistance = 0U;
    unsigned int observedCurrentDistance = 0U;

    QObject::connect(
        &handler,
        &ELinkPositionHandler::barometricRawEvent,
        [&](qlonglong, const int pressure, const int temperature) {
            ++barometricRawEvents;
            observedPressure = pressure;
            observedTemperature = temperature;
        });
    QObject::connect(
        &handler,
        &ELinkPositionHandler::magneticEvent,
        [&](qlonglong, const int x, const int y, const int z) {
            ++magneticEvents;
            observedMagnetic[0] = x;
            observedMagnetic[1] = y;
            observedMagnetic[2] = z;
        });
    const auto capturePosition = [&](const unsigned char status,
                                     const unsigned char satellites,
                                     const int latitude,
                                     const int longitude,
                                     const int altitude,
                                     const float vx,
                                     const float vy,
                                     const float vz,
                                     const float speed,
                                     const float course) {
        observedStatus = status;
        observedSatellites = satellites;
        observedLatitude = latitude;
        observedLongitude = longitude;
        observedAltitude = altitude;
        observedVelocityX = vx;
        observedVelocityY = vy;
        observedVelocityZ = vz;
        observedGroundSpeed = speed;
        observedGroundCourse = course;
    };
    QObject::connect(
        &handler,
        &ELinkPositionHandler::positionEvent,
        [&](qlonglong,
            const unsigned char status,
            const unsigned char satellites,
            const int latitude,
            const int longitude,
            const int altitude,
            const float vx,
            const float vy,
            const float vz,
            const float speed,
            const float course) {
            ++positionEvents;
            capturePosition(status,
                            satellites,
                            latitude,
                            longitude,
                            altitude,
                            vx,
                            vy,
                            vz,
                            speed,
                            course);
        });
    QObject::connect(
        &handler,
        &ELinkPositionHandler::backupPositionEvent,
        [&](qlonglong,
            const unsigned char status,
            const unsigned char satellites,
            const int latitude,
            const int longitude,
            const int altitude,
            const float vx,
            const float vy,
            const float vz,
            const float speed,
            const float course) {
            ++backupPositionEvents;
            capturePosition(status,
                            satellites,
                            latitude,
                            longitude,
                            altitude,
                            vx,
                            vy,
                            vz,
                            speed,
                            course);
        });
    QObject::connect(
        &handler,
        &ELinkPositionHandler::distanceSensorEvent,
        [&](const unsigned int minimum,
            const unsigned int maximum,
            const unsigned int current) {
            ++distanceEvents;
            observedMinimumDistance = minimum;
            observedMaximumDistance = maximum;
            observedCurrentDistance = current;
        });
    QObject::connect(&handler,
                     &ELinkPositionHandler::barometricAltEvent,
                     [&](qlonglong, const int altitude) {
                         ++barometricAltitudeEvents;
                         observedAltitude = altitude;
                     });
    QObject::connect(&handler,
                     &ELinkPositionHandler::launchAcceleration,
                     [&] { ++launchEvents; });

    QByteArray barometric;
    appendTestLittleEndian(barometric, 101325);
    appendTestLittleEndian(barometric, -273);
    link.feed(makeTestELinkFrame(0x00U, barometric));
    CHECK(barometricRawEvents == 1);
    CHECK(observedPressure == 101325);
    CHECK(observedTemperature == -273);

    QByteArray magnetic;
    appendTestLittleEndian(magnetic, -11);
    appendTestLittleEndian(magnetic, 22);
    appendTestLittleEndian(magnetic, -33);
    link.feed(makeTestELinkFrame(0x30U, magnetic));
    CHECK(magneticEvents == 1);
    CHECK(observedMagnetic[0] == -11);
    CHECK(observedMagnetic[1] == 22);
    CHECK(observedMagnetic[2] == -33);

    QByteArray position;
    position.append(static_cast<char>(4));
    position.append(static_cast<char>(17));
    appendTestLittleEndian(position, 123456);
    appendTestLittleEndian(position, 504303588);
    appendTestLittleEndian(position, 305038419);
    appendTestLittleEndian(position, 1.25F);
    appendTestLittleEndian(position, -2.5F);
    appendTestLittleEndian(position, 3.75F);
    appendTestLittleEndian(position, 21.5F);
    appendTestLittleEndian(position, 272.25F);
    link.feed(makeTestELinkFrame(0x40U, position));
    CHECK(positionEvents == 1);
    CHECK(observedStatus == 4U);
    CHECK(observedSatellites == 17U);
    CHECK(observedLatitude == 504303588);
    CHECK(observedLongitude == 305038419);
    CHECK(observedAltitude == 123456);
    CHECK(observedVelocityX == 1.25F);
    CHECK(observedVelocityY == -2.5F);
    CHECK(observedVelocityZ == 3.75F);
    CHECK(observedGroundSpeed == 21.5F);
    CHECK(observedGroundCourse == 272.25F);

    appendTestLittleEndian(position, std::uint32_t{111U});
    appendTestLittleEndian(position, std::uint32_t{222U});
    link.feed(makeTestELinkFrame(0x41U, position));
    CHECK(backupPositionEvents == 1);

    QByteArray distance;
    appendTestLittleEndian(distance, 1234);
    link.feed(makeTestELinkFrame(0x50U, distance));
    CHECK(distanceEvents == 1);
    CHECK(observedMinimumDistance == 100U);
    CHECK(observedMaximumDistance == 300000U);
    CHECK(observedCurrentDistance == 123U);

    QByteArray altitude;
    appendTestLittleEndian(altitude, -9876);
    link.feed(makeTestELinkFrame(0x51U, altitude));
    CHECK(barometricAltitudeEvents == 1);
    CHECK(observedAltitude == -9876);

    link.feed(makeTestELinkFrame(0x5fU, QByteArray(1, '\1')));
    link.feed(makeTestELinkFrame(0x5fU, QByteArray(1, '\1')));
    CHECK(launchEvents == 1);

    handler.setManualAccelerationEvent();
    CHECK(link.transmissions.size() == 1);
    checkTestELinkFrame(link.transmissions.back(),
                        0U,
                        0x03U,
                        QByteArray(1, '\0'));
    handler.sendAirspeed(14.5F);
    CHECK(link.transmissions.size() == 2);
    QByteArray expectedAirspeed;
    appendTestLittleEndian(expectedAirspeed, 14.5F);
    checkTestELinkFrame(link.transmissions.back(),
                        1U,
                        0x22U,
                        expectedAirspeed);
}

void testELinkFlowerHandler()
{
    ELinkCommunicator communicator;
    TestELink link;
    communicator.addLink(&link);
    ELinkFlowerHandler handler(&communicator);
    CHECK(!handler.isConnected());
    CHECK(handler.metaObject()->indexOfSignal(
              "currentAHRS(float,float,float,uchar,int,int,int,float,float,float)")
          >= 0);
    CHECK(handler.metaObject()->indexOfSignal("currentGPS(int,int)") >= 0);
    CHECK(handler.metaObject()->indexOfSlot(
              "sendCurrentCoordinate(qlonglong,int,int,float)")
          >= 0);

    int ahrsEvents = 0;
    int gpsEvents = 0;
    float observedRoll = 0.0F;
    float observedPitch = 0.0F;
    float observedYaw = 0.0F;
    unsigned char observedGnss = 0U;
    int observedLatitude = 0;
    int observedLongitude = 0;
    int observedAltitude = 0;
    float observedHeading = 0.0F;
    float observedGroundSpeed = 0.0F;
    float observedAirspeed = 0.0F;
    QObject::connect(
        &handler,
        &ELinkFlowerHandler::currentAHRS,
        [&](const float roll,
            const float pitch,
            const float yaw,
            const unsigned char gnss,
            const int latitude,
            const int longitude,
            const int altitude,
            const float heading,
            const float groundSpeed,
            const float airspeed) {
            ++ahrsEvents;
            observedRoll = roll;
            observedPitch = pitch;
            observedYaw = yaw;
            observedGnss = gnss;
            observedLatitude = latitude;
            observedLongitude = longitude;
            observedAltitude = altitude;
            observedHeading = heading;
            observedGroundSpeed = groundSpeed;
            observedAirspeed = airspeed;
        });
    QObject::connect(&handler,
                     &ELinkFlowerHandler::currentGPS,
                     [&](const int latitude, const int longitude) {
                         ++gpsEvents;
                         observedLatitude = latitude;
                         observedLongitude = longitude;
                     });

    const auto flowerBody = [](const std::uint8_t gnss) {
        QByteArray body;
        appendTestLittleEndian(body, std::int16_t{-123});
        appendTestLittleEndian(body, std::int16_t{456});
        appendTestLittleEndian(body, std::uint16_t{35999U});
        body.append(static_cast<char>(gnss));
        appendTestLittleEndian(body, 504303588);
        appendTestLittleEndian(body, 305038419);
        appendTestLittleEndian(body, std::uint16_t{9000U});
        appendTestLittleEndian(body, std::uint16_t{360U});
        appendTestLittleEndian(body, std::uint16_t{720U});
        appendTestLittleEndian(body, 123);
        return body;
    };
    link.feed(makeTestELinkFrame(0x68U, flowerBody(1U)));
    CHECK(ahrsEvents == 1);
    CHECK(gpsEvents == 1);
    CHECK(handler.isConnected());
    CHECK(near(observedRoll, -1.23, 1.0e-6));
    CHECK(near(observedPitch, 4.56, 1.0e-6));
    CHECK(near(observedYaw, 359.99, 1.0e-4));
    CHECK(observedGnss == 1U);
    CHECK(observedLatitude == 504303588);
    CHECK(observedLongitude == 305038419);
    CHECK(observedAltitude == 12300);
    CHECK(near(observedHeading, 90.0, 0.0));
    CHECK(near(observedGroundSpeed, 10.0, 0.0));
    CHECK(near(observedAirspeed, 20.0, 0.0));

    handler.sendCurrentCoordinate(123, 100, 200, 0.25F);
    CHECK(link.transmissions.isEmpty());
    link.feed(makeTestELinkFrame(0x68U, flowerBody(0U)));
    CHECK(ahrsEvents == 2);
    CHECK(gpsEvents == 1);
    handler.sendCurrentCoordinate(456, -100, -200, 0.75F);
    CHECK(link.transmissions.size() == 1);
    QByteArray expected;
    appendTestLittleEndian(expected, -100);
    appendTestLittleEndian(expected, -200);
    checkTestELinkFrame(link.transmissions.back(), 0U, 0x48U, expected);
}

void testELinkGimbalHandler()
{
    ELinkCommunicator communicator;
    TestELink link;
    communicator.addLink(&link);
    ELinkGimbalHandler handler(&communicator);
    CHECK(handler.metaObject()->indexOfSlot(
              "setServoPTZ(float,float,float,float,float,float,bool,bool,int)")
          >= 0);
    CHECK(handler.metaObject()->indexOfSlot(
              "setServoPTZ(float,float,float,float,float,float,bool,bool)")
          >= 0);
    CHECK(handler.metaObject()->indexOfSlot(
              "setServoPTZ(float,float,float,float,float,float,bool)")
          >= 0);
    CHECK(handler.metaObject()->indexOfSlot(
              "setServoPTZ(float,float,float,float,float,float)")
          >= 0);

    QByteArray report;
    report.append(static_cast<char>(1));
    appendTestLittleEndian(report, std::int16_t{-100});
    appendTestLittleEndian(report, std::int16_t{200});
    report.append(static_cast<char>(2));
    appendTestLittleEndian(report, std::int16_t{-300});
    appendTestLittleEndian(report, std::int16_t{400});
    link.feed(makeTestELinkFrame(0x87U, report));
    CHECK(link.transmissions.isEmpty());

    handler.setServoPTZ(1.0F,
                        12.5F,
                        3.0F,
                        4.0F,
                        5.0F,
                        6.0F);
    CHECK(link.transmissions.size() == 1);
    checkTestELinkFrame(link.transmissions[0],
                        0U,
                        0x88U,
                        QByteArray::fromHex("00e20400"));
    CHECK(processEventsUntil([&] { return link.transmissions.size() == 2; },
                             200));
    checkTestELinkFrame(link.transmissions[1],
                        1U,
                        0x88U,
                        QByteArray::fromHex("01000000"));

    handler.setServoPTZ(10.0F,
                        -45.0F,
                        20.0F,
                        1.0F,
                        2.0F,
                        3.0F,
                        true,
                        true,
                        7);
    CHECK(link.transmissions.size() == 3);
    checkTestELinkFrame(link.transmissions[2],
                        2U,
                        0x88U,
                        QByteArray::fromHex("00000001"));
    CHECK(processEventsUntil([&] { return link.transmissions.size() == 4; },
                             200));
    checkTestELinkFrame(link.transmissions[3],
                        3U,
                        0x88U,
                        QByteArray::fromHex("01000001"));
}

void testELinkShumodavHandler()
{
    ELinkCommunicator communicator;
    TestELink link;
    communicator.addLink(&link);
    ELinkShumodavHandler handler(&communicator);
    CHECK(handler.metaObject()->indexOfSignal(
              "target(short,short,short,short)")
          >= 0);

    int targetEvents = 0;
    short observedPowerAzimuth = 0;
    short observedPowerElevation = 0;
    short observedAngleAzimuth = 0;
    short observedAngleElevation = 0;
    QObject::connect(&handler,
                     &ELinkShumodavHandler::target,
                     [&](const short powerAzimuth,
                         const short powerElevation,
                         const short angleAzimuth,
                         const short angleElevation) {
                         ++targetEvents;
                         observedPowerAzimuth = powerAzimuth;
                         observedPowerElevation = powerElevation;
                         observedAngleAzimuth = angleAzimuth;
                         observedAngleElevation = angleElevation;
                     });

    const auto item = [](const short statusAzimuth,
                         const short statusElevation,
                         const short powerAzimuth,
                         const short powerElevation,
                         const short angleAzimuth,
                         const short angleElevation) {
        QByteArray result(26, '\0');
        const auto write = [&](const int offset, const short value) {
            const auto raw = static_cast<std::uint16_t>(value);
            result[offset] = static_cast<char>(raw & 0xffU);
            result[offset + 1] = static_cast<char>((raw >> 8U) & 0xffU);
        };
        write(2, statusAzimuth);
        write(4, statusElevation);
        write(10, powerAzimuth);
        write(12, powerElevation);
        write(18, angleAzimuth);
        write(20, angleElevation);
        return result;
    };

    QByteArray body(6, '\0');
    body[2] = 2;
    body.append(item(1, 1, 10, 20, 100, 200));
    body.append(item(1, 1, 30, 40, 300, 400));
    link.feed(makeTestELinkFrame(0xc0U, body));
    CHECK(targetEvents == 1);
    CHECK(observedPowerAzimuth == 30);
    CHECK(observedPowerElevation == 40);
    CHECK(observedAngleAzimuth == 300);
    CHECK(observedAngleElevation == 400);

    body = QByteArray(6, '\0');
    body[2] = 1;
    body.append(item(0, 1, 500, 500, 1, 2));
    link.feed(makeTestELinkFrame(0xc0U, body));
    CHECK(targetEvents == 1);
}

#ifdef EPATHFINDER_HAS_MAVLINK
void testELinkTelemetry()
{
    ELinkCommunicator communicator;
    TestELink link;
    communicator.addLink(&link);
    ELinkTelemetry telemetry(&communicator, 7U);

    CHECK(telemetry.metaObject()->indexOfSignal("telemetryConnected()") >= 0);
    CHECK(telemetry.metaObject()->indexOfSignal(
              "planeTarget(PlaneTarget)")
          >= 0);
    CHECK(telemetry.metaObject()->indexOfSignal(
              "remoteControlEnable(uchar,int,int,int,uchar)")
          >= 0);
    CHECK(telemetry.telemetryPlaneId() == 0U);
    telemetry.setTelemetryPlaneId(0x3456U);
    CHECK(telemetry.telemetryPlaneId() == 0x3456U);
    telemetry.setTelemetryPlaneUid(QStringLiteral("plane-A"));
    CHECK(telemetry.telemetryPlaneUid() == QStringLiteral("plane-A"));
    CHECK(!telemetry.isConnected());

    telemetry.getTelemetryStatus();
    CHECK(link.transmissions.size() == 1);
    checkTestELinkFrame(link.transmissions.at(0),
                        0U,
                        0x8bU,
                        QByteArray::fromHex("01"));
    telemetry.setTelemetryConfig(0x12U, 0x34U);
    CHECK(link.transmissions.size() == 2);
    checkTestELinkFrame(link.transmissions.at(1),
                        1U,
                        0x8bU,
                        QByteArray::fromHex("001234"));

    telemetry.sendPosAtt(1001,
                         504303588,
                         305038419,
                         123450,
                         1.25F,
                         -2.5F,
                         179.75F,
                         31.5F);
    const QByteArray posAtt =
        ELinkTelemetryTestProbe::queued(telemetry, 0x01U);
    CHECK(posAtt.size() == 41);
    CHECK(posAtt.left(5) == QByteArray::fromHex("E00E563401"));
    CHECK(readTestLittleEndian<qlonglong>(posAtt, 5) == 1001);
    CHECK(readTestLittleEndian<int>(posAtt, 13) == 504303588);
    CHECK(readTestLittleEndian<int>(posAtt, 17) == 305038419);
    CHECK(readTestLittleEndian<int>(posAtt, 21) == 123450);
    CHECK(readTestLittleEndian<float>(posAtt, 25) == 1.25F);
    CHECK(readTestLittleEndian<float>(posAtt, 29) == -2.5F);
    CHECK(readTestLittleEndian<float>(posAtt, 33) == 179.75F);
    CHECK(readTestLittleEndian<float>(posAtt, 37) == 31.5F);

    telemetry.sendGPSPos(1002, 1, 2, 3);
    const QByteArray gps =
        ELinkTelemetryTestProbe::queued(telemetry, 0x11U);
    CHECK(gps.size() == 25);
    CHECK(gps.left(5) == QByteArray::fromHex("E00E563411"));
    CHECK(readTestLittleEndian<qlonglong>(gps, 5) == 1002);
    CHECK(readTestLittleEndian<int>(gps, 13) == 1);
    CHECK(readTestLittleEndian<int>(gps, 17) == 2);
    CHECK(readTestLittleEndian<int>(gps, 21) == 3);

    telemetry.sendGPSStatus(1003, 4U, 5U, 6U);
    const QByteArray gpsStatus =
        ELinkTelemetryTestProbe::queued(telemetry, 0x02U);
    CHECK(gpsStatus.size() == 16);
    CHECK(gpsStatus.right(3) == QByteArray::fromHex("040506"));

    telemetry.sendPlanModeInfo(1004, 7U, 800U, -9.5F);
    const QByteArray planMode =
        ELinkTelemetryTestProbe::queued(telemetry, 0x34U);
    CHECK(planMode.size() == 20);
    CHECK(static_cast<unsigned char>(planMode.at(13)) == 7U);
    CHECK(readTestLittleEndian<std::uint16_t>(planMode, 14) == 800U);
    CHECK(readTestLittleEndian<float>(planMode, 16) == -9.5F);

    telemetry.sendRadarEmulationInfo(
        1005, 11, 22, 33, 44.5F, 55.25F);
    const QByteArray radar =
        ELinkTelemetryTestProbe::queued(telemetry, 0x13U);
    CHECK(radar.size() == 35);
    CHECK(radar.left(5) == QByteArray::fromHex("E00E563413"));
    CHECK(readTestLittleEndian<std::uint16_t>(radar, 33) == 0U);

    Plan plan;
    plan.items.append(PlanItem{});
    PlanItem searchItem;
    searchItem.latitude = 50.1;
    searchItem.longitude = 30.2;
    searchItem.altitude = 123.0;
    searchItem.type = PlanItemType::Search;
    searchItem.radius = 444;
    searchItem.timeout = 555;
    searchItem.roadType = PlanRoadType::Search;
    searchItem.trustGps = true;
    plan.items.append(searchItem);
    telemetry.sendPlan(1006, plan);
    const QByteArray planMessage =
        ELinkTelemetryTestProbe::queued(telemetry, 0x12U);
    CHECK(planMessage.size() > 15);
    CHECK(planMessage.left(5) == QByteArray::fromHex("E00E563412"));
    CHECK(readTestLittleEndian<qlonglong>(planMessage, 5) == 1006);
    const std::uint16_t jsonLength =
        readTestLittleEndian<std::uint16_t>(planMessage, 13);
    CHECK(jsonLength == planMessage.size() - 15);
    const QJsonObject planJson =
        QJsonDocument::fromJson(planMessage.mid(15)).object();
    const QJsonArray planNodes = planJson.value(QStringLiteral("nodes")).toArray();
    CHECK(planNodes.size() == 1);
    CHECK(planNodes.at(0).toObject().value(QStringLiteral("radius")).toInt()
          == 444);
    CHECK(planNodes.at(0).toObject().value(QStringLiteral("trust_gps")).toBool());
    CHECK(ELinkTelemetryTestProbe::queueSize(telemetry) == 6);

    int targetEvents = 0;
    PlaneTarget observedTarget;
    QObject::connect(&telemetry,
                     &ELinkTelemetry::planeTarget,
                     [&](const PlaneTarget target) {
                         ++targetEvents;
                         observedTarget = target;
                     });
    QByteArray targetBody(3, '\0');
    targetBody.append(static_cast<char>(0x13));
    appendTestLittleEndian(targetBody,
                           static_cast<quint64>(0x0102030405060708ULL));
    appendTestLittleEndian(targetBody, static_cast<std::int32_t>(504303588));
    appendTestLittleEndian(targetBody, static_cast<std::int32_t>(305038419));
    appendTestLittleEndian(targetBody, static_cast<std::int32_t>(123450));
    appendTestLittleEndian(targetBody, 71.5F);
    appendTestLittleEndian(targetBody, 42.25F);
    link.feed(makeTestELinkFrame(0x89U, targetBody));
    CHECK(targetEvents == 1);
    CHECK(near(observedTarget.coordinate.latitude(), 50.4303588, 1e-8));
    CHECK(near(observedTarget.coordinate.longitude(), 30.5038419, 1e-8));
    CHECK(near(observedTarget.coordinate.altitude(), 123.45, 1e-6));
    // The direct ELink path parses but does not assign either float.
    CHECK(observedTarget.azimuth == 0.0F);
    CHECK(observedTarget.groundSpeed == 0.0F);
    CHECK(!observedTarget.valid);
    link.feed(makeTestELinkFrame(0x89U, targetBody));
    CHECK(targetEvents == 1);

    quint16 basePort = 0U;
    for (int attempt = 0; attempt < 100 && basePort == 0U; ++attempt) {
        QUdpSocket first;
        CHECK(first.bind(QHostAddress::LocalHost, 0));
        const quint16 candidate = first.localPort();
        QUdpSocket second;
        if (candidate != std::numeric_limits<quint16>::max()
            && second.bind(QHostAddress::LocalHost,
                           static_cast<quint16>(candidate + 1U))) {
            basePort = candidate;
        }
    }
    CHECK(basePort != 0U);
    telemetry.Init(QStringLiteral("127.0.0.1"), basePort);

    int peekEvents = 0;
    QObject::connect(&telemetry,
                     &ELinkTelemetry::peekAtGPS,
                     [&] { ++peekEvents; });
    QUdpSocket sender;
    CHECK(sender.writeDatagram(QByteArray::fromHex("E00E77"),
                               QHostAddress::LocalHost,
                               basePort)
          == 3);
    CHECK(processEventsUntil([&] { return peekEvents == 1; }, 200));

    int remoteEvents = 0;
    unsigned char observedMode = 0U;
    int observedRoll = 0;
    int observedPitch = 0;
    int observedThrottle = 0;
    unsigned char observedOptions = 0U;
    QObject::connect(
        &telemetry,
        &ELinkTelemetry::remoteControlEnable,
        [&](const unsigned char mode,
            const int roll,
            const int pitch,
            const int throttle,
            const unsigned char options) {
            ++remoteEvents;
            observedMode = mode;
            observedRoll = roll;
            observedPitch = pitch;
            observedThrottle = throttle;
            observedOptions = options;
        });

    QByteArray sidMessage = QByteArray::fromHex("E00E0001");
    sidMessage.append("plane-A");
    appendTestLittleEndian(sidMessage, static_cast<std::uint16_t>(0x2244U));
    CHECK(sender.writeDatagram(sidMessage,
                               QHostAddress::LocalHost,
                               static_cast<quint16>(basePort + 1U))
          == sidMessage.size());

    QByteArray remoteMessage = QByteArray::fromHex("E00E0006");
    appendTestLittleEndian(remoteMessage,
                           static_cast<std::uint16_t>(0x2244U));
    remoteMessage.append(static_cast<char>(2));
    appendTestLittleEndian(remoteMessage, static_cast<std::int16_t>(-100));
    appendTestLittleEndian(remoteMessage, static_cast<std::int16_t>(200));
    remoteMessage.append(static_cast<char>(23));
    remoteMessage.append(static_cast<char>(0xa5));
    CHECK(sender.writeDatagram(remoteMessage,
                               QHostAddress::LocalHost,
                               static_cast<quint16>(basePort + 1U))
          == remoteMessage.size());
    CHECK(processEventsUntil([&] { return remoteEvents == 1; }, 200));
    CHECK(observedMode == 2U);
    CHECK(observedRoll == -100);
    CHECK(observedPitch == 200);
    CHECK(observedThrottle == 1230);
    CHECK(observedOptions == 0xa5U);
}
#endif

void testVNav()
{
    QUdpSocket receivePortProbe;
    CHECK(receivePortProbe.bind(QHostAddress::LocalHost, 0));
    const quint16 receivePort = receivePortProbe.localPort();
    CHECK(receivePort != 0U);
    receivePortProbe.close();

    QUdpSocket output;
    CHECK(output.bind(QHostAddress::LocalHost, 0));
    const quint16 outputPort = output.localPort();
    CHECK(outputPort != 0U);

    TestVNav vnav(receivePort, outputPort);
    CHECK(vnav.metaObject()->indexOfSignal(
              "currentPos(qlonglong,int,int,float)")
          >= 0);
    CHECK(vnav.metaObject()->indexOfSignal("statusChanged(bool)") >= 0);
    CHECK(vnav.metaObject()->indexOfSignal(
              "vehicleDetected(qlonglong,QList<VehicleTarget>)")
          >= 0);
    CHECK(vnav.metaObject()->indexOfSignal("checkPlanResult(bool,bool)")
          >= 0);
    CHECK(vnav.metaObject()->indexOfSignal(
              "imageTarget(qlonglong,short,short,uchar)")
          >= 0);
    CHECK(vnav.metaObject()->indexOfSignal(
              "ahrs(qlonglong,int,int,uint,float,float,float,uchar)")
          >= 0);
    CHECK(vnav.metaObject()->indexOfSignal("yawOffset(qlonglong,float)")
          >= 0);
    CHECK(vnav.metaObject()->indexOfSlot(
              "sendCurrentIMU(int,int,int,float,short,short,float,float,float,float,int,int)")
          >= 0);
    CHECK(vnav.metaObject()->indexOfSlot(
              "setRoadPoints(qlonglong,QList<QPoint>)")
          >= 0);

    QByteArray datagram;
    vnav.start();
    CHECK(receiveUdpDatagram(output, datagram));
    CHECK(datagram == QByteArray::fromHex("e00e01"));
    vnav.start();
    CHECK(!output.waitForReadyRead(20));

    vnav.reset();
    CHECK(receiveUdpDatagram(output, datagram));
    CHECK(datagram == QByteArray::fromHex("e00e02"));

    vnav.setMode(1U, 504303588, 305038419);
    CHECK(receiveUdpDatagram(output, datagram));
    CHECK(datagram.size() == 12);
    CHECK(datagram.left(4) == QByteArray::fromHex("e00e0401"));
    CHECK(readTestLittleEndian<int>(datagram, 4) == 504303588);
    CHECK(readTestLittleEndian<int>(datagram, 8) == 305038419);

    vnav.sendCurrentIMU(504300000,
                        305000000,
                        12345,
                        20.0F,
                        12,
                        -13,
                        1.5F,
                        -2.5F,
                        5.0F,
                        3.0F,
                        504299999,
                        304999999);
    CHECK(receiveUdpDatagram(output, datagram));
    CHECK(datagram.size() == 51);
    CHECK(datagram.left(3) == QByteArray::fromHex("e00e03"));

    vnav.sendCurrentIMU(504300001,
                        305000001,
                        12346,
                        30.0F,
                        14,
                        -15,
                        1.25F,
                        -2.25F,
                        5.0F,
                        4.0F,
                        504299998,
                        304999998);
    CHECK(receiveUdpDatagram(output, datagram));
    CHECK(datagram.size() == 51);
    CHECK(readTestLittleEndian<int>(datagram, 11) == 504300001);
    CHECK(readTestLittleEndian<int>(datagram, 15) == 305000001);
    CHECK(readTestLittleEndian<int>(datagram, 19) == 12346);
    CHECK(readTestLittleEndian<std::int16_t>(datagram, 23) == 14);
    CHECK(readTestLittleEndian<std::int16_t>(datagram, 25) == -15);
    CHECK(near(readTestLittleEndian<float>(datagram, 27), 1.25, 0.0));
    CHECK(near(readTestLittleEndian<float>(datagram, 31), -2.25, 0.0));
    CHECK(near(readTestLittleEndian<float>(datagram, 35), 35.0, 0.0));
    CHECK(near(readTestLittleEndian<float>(datagram, 39), 4.0, 0.0));
    CHECK(readTestLittleEndian<int>(datagram, 43) == 504299998);
    CHECK(readTestLittleEndian<int>(datagram, 47) == 304999998);
    const qlonglong imuTime = readTestLittleEndian<qlonglong>(datagram, 3);
    CHECK(std::abs(QDateTime::currentMSecsSinceEpoch() - imuTime) < 1000);

    vnav.setRoadPoints(123456789,
                       {QPoint(100, -200), QPoint(32767, -32768)});
    CHECK(receiveUdpDatagram(output, datagram));
    CHECK(datagram.size() == 19);
    CHECK(datagram.left(3) == QByteArray::fromHex("e00e07"));
    CHECK(readTestLittleEndian<qlonglong>(datagram, 3) == 123456789);
    CHECK(readTestLittleEndian<std::int16_t>(datagram, 11) == 100);
    CHECK(readTestLittleEndian<std::int16_t>(datagram, 13) == -200);
    CHECK(readTestLittleEndian<std::int16_t>(datagram, 15) == 32767);
    CHECK(readTestLittleEndian<std::int16_t>(datagram, 17) == -32768);

    vnav.stop();
    CHECK(receiveUdpDatagram(output, datagram));
    CHECK(datagram == QByteArray::fromHex("e00e00"));
    vnav.setMode(1U, 1, 2);
    CHECK(!output.waitForReadyRead(20));
    vnav.setMode(4U, 1, 2);
    CHECK(receiveUdpDatagram(output, datagram));
    CHECK(datagram == QByteArray::fromHex("e00e0404"));

    int currentPosEvents = 0;
    qlonglong observedTime = 0;
    int observedLat = 0;
    int observedLon = 0;
    float observedConfidence = 0.0F;
    QObject::connect(&vnav,
                     &VNav::currentPos,
                     [&](const qlonglong time,
                         const int lat,
                         const int lon,
                         const float confidence) {
                         ++currentPosEvents;
                         observedTime = time;
                         observedLat = lat;
                         observedLon = lon;
                         observedConfidence = confidence;
                     });

    int statusEvents = 0;
    bool observedStatus = false;
    QObject::connect(&vnav,
                     &VNav::statusChanged,
                     [&](const bool active) {
                         ++statusEvents;
                         observedStatus = active;
                     });

    int vehicleEvents = 0;
    qlonglong vehicleTime = 0;
    QList<VehicleTarget> observedTargets;
    QObject::connect(&vnav,
                     &VNav::vehicleDetected,
                     [&](const qlonglong time,
                         const QList<VehicleTarget>& targets) {
                         ++vehicleEvents;
                         vehicleTime = time;
                         observedTargets = targets;
                     });

    int imageEvents = 0;
    qlonglong imageTime = 0;
    short imageX = 0;
    short imageY = 0;
    uchar imageType = 0U;
    QObject::connect(&vnav,
                     &VNav::imageTarget,
                     [&](const qlonglong time,
                         const short x,
                         const short y,
                         const uchar type) {
                         ++imageEvents;
                         imageTime = time;
                         imageX = x;
                         imageY = y;
                         imageType = type;
                     });

    int ahrsEvents = 0;
    int ahrsLat = 0;
    int ahrsLon = 0;
    uint ahrsAlt = 0U;
    float ahrsRoll = 0.0F;
    float ahrsPitch = 0.0F;
    float ahrsYaw = 0.0F;
    uchar ahrsStatus = 0U;
    QObject::connect(&vnav,
                     &VNav::ahrs,
                     [&](qlonglong,
                         const int lat,
                         const int lon,
                         const uint alt,
                         const float roll,
                         const float pitch,
                         const float yaw,
                         const uchar status) {
                         ++ahrsEvents;
                         ahrsLat = lat;
                         ahrsLon = lon;
                         ahrsAlt = alt;
                         ahrsRoll = roll;
                         ahrsPitch = pitch;
                         ahrsYaw = yaw;
                         ahrsStatus = status;
                     });

    int yawEvents = 0;
    qlonglong yawTime = 0;
    float observedYawOffset = 0.0F;
    QObject::connect(&vnav,
                     &VNav::yawOffset,
                     [&](const qlonglong time, const float offset) {
                         ++yawEvents;
                         yawTime = time;
                         observedYawOffset = offset;
                     });

    int planEvents = 0;
    bool observedValidVNav = false;
    bool observedValidVNav2 = false;
    QObject::connect(&vnav,
                     &VNav::checkPlanResult,
                     [&](const bool validVNav, const bool validVNav2) {
                         ++planEvents;
                         observedValidVNav = validVNav;
                         observedValidVNav2 = validVNav2;
                     });

    QUdpSocket injector;
    const auto inject = [&](const QByteArray& packet) {
        CHECK(injector.writeDatagram(packet,
                                     QHostAddress::LocalHost,
                                     receivePort)
              == packet.size());
    };

    QByteArray input = QByteArray::fromHex("e00e01");
    appendTestLittleEndian(input, 504300100);
    appendTestLittleEndian(input, 305000100);
    input.append(static_cast<char>(128));
    inject(input);
    CHECK(processEventsUntil([&] { return currentPosEvents == 1; }));
    CHECK(observedLat == 504300100);
    CHECK(observedLon == 305000100);
    CHECK(near(observedConfidence, 128.0 / 255.0, 1.0e-6));
    CHECK(std::abs(QDateTime::currentMSecsSinceEpoch() - observedTime) < 1000);

    input = QByteArray::fromHex("e00e01");
    appendTestLittleEndian(input, static_cast<qlonglong>(987654321));
    appendTestLittleEndian(input, 504300200);
    appendTestLittleEndian(input, 305000200);
    input.append(static_cast<char>(255));
    inject(input);
    CHECK(processEventsUntil([&] { return currentPosEvents == 2; }));
    CHECK(observedTime == 987654321);
    CHECK(observedLat == 504300200);
    CHECK(observedLon == 305000200);
    CHECK(near(observedConfidence, 1.0, 0.0));

    input = QByteArray::fromHex("e00e02");
    appendTestLittleEndian(input, 504300300);
    appendTestLittleEndian(input, 305000300);
    input.append(static_cast<char>(1));
    inject(input);
    (void)processEventsUntil([] { return false; }, 20);
    CHECK(currentPosEvents == 2);
    vnav.setOdoEnabled(true);
    inject(input);
    CHECK(processEventsUntil([&] { return currentPosEvents == 3; }));
    CHECK(observedLat == 504300300);
    CHECK(observedLon == 305000300);
    CHECK(near(observedConfidence, 1.0, 0.0));

    input = QByteArray::fromHex("e00e00");
    input.append(static_cast<char>(0));
    inject(input);
    CHECK(processEventsUntil([&] {
        vnav.runTimerEvent();
        return statusEvents == 1;
    }));
    CHECK(observedStatus);
    input[3] = static_cast<char>(1);
    inject(input);
    CHECK(processEventsUntil([&] {
        vnav.runTimerEvent();
        return statusEvents == 2;
    }));
    CHECK(!observedStatus);

    input = QByteArray::fromHex("e00e05");
    appendTestLittleEndian(input, static_cast<std::uint16_t>(1));
    input.append(static_cast<char>(7));
    appendTestLittleEndian(input, -11.5F);
    appendTestLittleEndian(input, 42.25F);
    inject(input);
    CHECK(processEventsUntil([&] { return vehicleEvents == 1; }));
    CHECK(vehicleTime > 0);
    CHECK(observedTargets.size() == 1);
    CHECK(observedTargets[0].type == 7U);
    CHECK(near(observedTargets[0].elevation, -11.5, 0.0));
    CHECK(near(observedTargets[0].azimuth, 42.25, 0.0));

    input = QByteArray::fromHex("e00e08");
    appendTestLittleEndian(input, static_cast<qlonglong>(111222333));
    appendTestLittleEndian(input, static_cast<std::int16_t>(-123));
    appendTestLittleEndian(input, static_cast<std::int16_t>(456));
    input.append(static_cast<char>(9));
    inject(input);
    CHECK(processEventsUntil([&] { return imageEvents == 1; }));
    CHECK(imageTime == 111222333);
    CHECK(imageX == -123);
    CHECK(imageY == 456);
    CHECK(imageType == 9U);

    input = QByteArray::fromHex("e00e09");
    appendTestLittleEndian(input, static_cast<qlonglong>(444555666));
    appendTestLittleEndian(input, static_cast<std::int16_t>(123));
    appendTestLittleEndian(input, static_cast<std::int16_t>(-456));
    inject(input);
    CHECK(processEventsUntil([&] { return imageEvents == 2; }));
    CHECK(imageTime == 444555666);
    CHECK(imageX == 123);
    CHECK(imageY == -456);
    CHECK(imageType == 2U);

    input = QByteArray::fromHex("e00e0a");
    appendTestLittleEndian(input, imuTime);
    appendTestLittleEndian(input, 504300400);
    appendTestLittleEndian(input, 305000400);
    appendTestLittleEndian(input, static_cast<std::uint32_t>(54321));
    appendTestLittleEndian(input, 1.5F);
    appendTestLittleEndian(input, -2.5F);
    appendTestLittleEndian(input, 50.0F);
    input.append(static_cast<char>(3));
    inject(input);
    CHECK(processEventsUntil([&] { return ahrsEvents == 1; }));
    CHECK(ahrsLat == 504300400);
    CHECK(ahrsLon == 305000400);
    CHECK(ahrsAlt == 54321U);
    CHECK(near(ahrsRoll, 1.5, 0.0));
    CHECK(near(ahrsPitch, -2.5, 0.0));
    CHECK(near(ahrsYaw, 50.0, 0.0));
    CHECK(ahrsStatus == 3U);
    CHECK(yawEvents == 1);
    CHECK(yawTime == imuTime);
    CHECK(near(observedYawOffset, 15.0, 1.0e-6));

    const QList<QGeoCoordinate> plan{
        QGeoCoordinate(50.43, 30.50),
        QGeoCoordinate(50.44, 30.51)};
    vnav.checkPlan(plan);
    CHECK(receiveUdpDatagram(output, datagram));
    CHECK(datagram.size() == 19);
    CHECK(datagram.left(3) == QByteArray::fromHex("e00e06"));
    const int aLat = static_cast<int>(plan[0].latitude() * 10000000.0);
    const int aLon = static_cast<int>(plan[0].longitude() * 10000000.0);
    const int bLat = static_cast<int>(plan[1].latitude() * 10000000.0);
    const int bLon = static_cast<int>(plan[1].longitude() * 10000000.0);
    CHECK(readTestLittleEndian<int>(datagram, 3) == aLat);
    CHECK(readTestLittleEndian<int>(datagram, 7) == aLon);
    CHECK(readTestLittleEndian<int>(datagram, 11) == bLat);
    CHECK(readTestLittleEndian<int>(datagram, 15) == bLon);

    input = QByteArray::fromHex("e00e06");
    appendTestLittleEndian(input, aLat);
    appendTestLittleEndian(input, aLon);
    appendTestLittleEndian(input, bLat);
    appendTestLittleEndian(input, bLon);
    input.append(static_cast<char>(0));
    input.append(static_cast<char>(0));
    inject(input);
    CHECK(processEventsUntil([&] { return planEvents == 1; }));
    CHECK(observedValidVNav);
    CHECK(observedValidVNav2);

    vnav.checkPlan(plan);
    input[19] = static_cast<char>(1);
    inject(input);
    CHECK(processEventsUntil([&] { return planEvents == 2; }));
    CHECK(!observedValidVNav);
    CHECK(!observedValidVNav2);

    const int oldAhrsEvents = ahrsEvents;
    inject(QByteArray::fromHex("e00e0a00"));
    (void)processEventsUntil([] { return false; }, 20);
    CHECK(ahrsEvents == oldAhrsEvents);
}

void testNetworkLinks()
{
    UdpLink udp(0, QStringLiteral("127.0.0.1"), 14550);
    CHECK(udp.metaObject()->indexOfProperty("isUp") >= 0);
    CHECK(udp.metaObject()->indexOfProperty("rxPort") >= 0);
    CHECK(udp.metaObject()->indexOfProperty("address") >= 0);
    CHECK(udp.metaObject()->indexOfProperty("txPort") >= 0);
    CHECK(udp.rxPort() == 0);
    CHECK(udp.address() == QStringLiteral("127.0.0.1"));
    CHECK(udp.txPort() == 14550);

    int upEvents = 0;
    bool lastState = false;
    QObject::connect(&udp, &AbstractLink::upChanged, [&](const bool state) {
        ++upEvents;
        lastState = state;
    });
    udp.up();
    CHECK(udp.isUp());
    CHECK(upEvents == 1 && lastState);
    udp.down();
    CHECK(!udp.isUp());
    CHECK(upEvents == 2 && !lastState);

    udp.setAddress(QStringLiteral("localhost"));
    udp.setTxPort(14551);
    udp.setRxPort(0);
    CHECK(udp.address() == QStringLiteral("localhost"));
    CHECK(udp.txPort() == 14551);

    TcpLink tcp(QStringLiteral("127.0.0.1"), 65535);
    CHECK(!tcp.isUp());
    tcp.down();

    SerialLink serial(QStringLiteral("/dev/epathfinder-not-present"),
                      QSerialPort::Baud115200);
    CHECK(!serial.isUp());
    CHECK(serial.portName() == QStringLiteral("epathfinder-not-present"));
    CHECK(serial.baudRate() == QSerialPort::Baud115200);
    serial.setPortName(QStringLiteral("/dev/another-not-present"));
    CHECK(serial.portName() == QStringLiteral("another-not-present"));
    serial.setBaudRate(QSerialPort::Baud57600);
    CHECK(serial.baudRate() == QSerialPort::Baud57600);
}

void testApplicationSupportAndInterception()
{
    ELogger logger;
    CHECK(logger.metaObject()->indexOfMethod("setEnabled()") >= 0);
    CHECK(logger.metaObject()->indexOfMethod("setDisabled()") >= 0);
    CHECK(logger.metaObject()->indexOfMethod("flushLog()") >= 0);
    CHECK(logger.curRootLogDir()
          == QStringLiteral("/home/videoview/EPathfinder_logs"));
    logger.setDisabled();
    CHECK(!logger.isEnabled());
    logger.setEnabled();
    CHECK(logger.isEnabled());

    RemoteController remote;
    CHECK(remote.metaObject()->indexOfSignal("enableControl()") >= 0);
    CHECK(remote.metaObject()->indexOfSignal("disableControl()") >= 0);
    CHECK(remote.metaObject()->indexOfSignal(
              "controlValues(int,int,int)")
          >= 0);
    int enabled = 0;
    int disabled = 0;
    int controlEvents = 0;
    int roll = 0;
    int pitch = 0;
    int throttle = 0;
    QObject::connect(&remote, &RemoteController::enableControl,
                     [&] { ++enabled; });
    QObject::connect(&remote, &RemoteController::disableControl,
                     [&] { ++disabled; });
    QObject::connect(&remote,
                     &RemoteController::controlValues,
                     [&](const int r, const int p, const int t) {
                         ++controlEvents;
                         roll = r;
                         pitch = p;
                         throttle = t;
                     });
    RemoteControllerTestProbe::receive(
        remote, QByteArray::fromHex("00e00e01e00e02"), 1000);
    CHECK(enabled == 1);
    CHECK(disabled == 1);
    QByteArray controls = QByteArray::fromHex("e00e03");
    appendTestLittleEndian<quint16>(controls, 1000U);
    appendTestLittleEndian<quint16>(controls, 1100U);
    appendTestLittleEndian<quint16>(controls, 1200U);
    RemoteControllerTestProbe::receive(remote, controls.left(4), 1001);
    RemoteControllerTestProbe::receive(remote, controls.mid(4), 1002);
    CHECK(controlEvents == 1);
    CHECK(roll == 1000 && pitch == 1100 && throttle == 1200);

    EInterceptor interceptor;
    CHECK(interceptor.metaObject()->indexOfSignal(
              "planeTarget(PlaneTarget)")
          >= 0);
    CHECK(interceptor.metaObject()->indexOfSignal(
              "lidarPlaneTarget(QList<PlaneCoord>)")
          >= 0);
    int targetEvents = 0;
    PlaneTarget target;
    QObject::connect(&interceptor,
                     &EInterceptor::planeTarget,
                     [&](const PlaneTarget& value) {
                         ++targetEvents;
                         target = value;
                     });
    QByteArray radar(35, '\0');
    radar[0] = static_cast<char>(0xe0);
    radar[1] = static_cast<char>(0x0e);
    radar[2] = static_cast<char>(0x13);
    qToLittleEndian<qint32>(500'000'000,
                            reinterpret_cast<uchar*>(radar.data() + 13));
    qToLittleEndian<qint32>(300'000'000,
                            reinterpret_cast<uchar*>(radar.data() + 17));
    qToLittleEndian<qint32>(123'000,
                            reinterpret_cast<uchar*>(radar.data() + 21));
    const float azimuth = 42.5F;
    const float speed = 17.25F;
    std::memcpy(radar.data() + 25, &azimuth, sizeof(azimuth));
    std::memcpy(radar.data() + 29, &speed, sizeof(speed));
    EInterceptorTestProbe::receive(interceptor, radar, 5000);
    CHECK(targetEvents == 1);
    CHECK(target.time == 4800);
    CHECK(near(target.coordinate.latitude(), 50.0, 1.0e-9));
    CHECK(near(target.coordinate.longitude(), 30.0, 1.0e-9));
    CHECK(near(target.coordinate.altitude(), 123.0, 1.0e-9));
    CHECK(target.azimuth == azimuth);
    CHECK(target.groundSpeed == speed);
    CHECK(!target.valid);
}

void testScout()
{
    static_assert(sizeof(ECoord) == 0x20U);
    static_assert(sizeof(EPosition) == 0x58U);
    static_assert(sizeof(EAttitude) == 0x30U);
    static_assert(sizeof(ERawGpsPosition) == 0x28U);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    static_assert(sizeof(RoadValidateResult) == 0x58U);
#endif

    EScout scout;
    const QMetaObject* const meta = scout.metaObject();
    CHECK(meta->indexOfSignal("timeSynced()") >= 0);
    CHECK(meta->indexOfSignal(
              "imuCorrected(float,float,int,IMUCorrectSource)")
          >= 0);
    CHECK(meta->indexOfMethod("setTimeOffset(qlonglong,int)") >= 0);
    CHECK(meta->indexOfMethod(
              "setEPosition(qlonglong,uchar,uchar,int,int,int,float,float,float,float,float)")
          >= 0);
    CHECK(meta->indexOfMethod("lastRawIMU()") >= 0);

    int syncEvents = 0;
    int validEvents = 0;
    int invalidEvents = 0;
    QObject::connect(&scout, &EScout::timeSynced, [&] { ++syncEvents; });
    QObject::connect(&scout, &EScout::valid, [&] { ++validEvents; });
    QObject::connect(&scout, &EScout::invalid, [&] { ++invalidEvents; });

    constexpr qlonglong timeOffset = 1'000'000;
    constexpr unsigned int bootTime = 1'000U;
    constexpr qlonglong sampleTime = timeOffset + bootTime;
    scout.setTimeOffset(timeOffset, 12);
    CHECK(syncEvents == 1);
    CHECK(scout.mav2SysTime(bootTime) == sampleTime);
    CHECK(scout.sys2MavTime(sampleTime) == bootTime);

    scout.setSatelliteCount(3);
    CHECK(scout.currentState() == 0);
    scout.setSatelliteCount(4);
    CHECK(validEvents == 1);
    CHECK(invalidEvents == 0);
    CHECK(scout.currentState() == 1);

    constexpr int latitude = 500'000'000;
    constexpr int longitude = 300'000'000;
    scout.setPosition(bootTime,
                      latitude,
                      longitude,
                      100'000,
                      200'000,
                      100,
                      200,
                      50,
                      1234);
    CHECK(scout.lastPositionTime() == sampleTime);
    CHECK(near(scout.groundSpeed(), 3.0, 1.0e-6));
    CHECK(near(scout.gpsGroundSpeed(), std::sqrt(5.0), 1.0e-5));
    CHECK(near(scout.gpsAzimuth(), 12.34, 1.0e-5));

    const EPosition projected = scout.position(sampleTime + 1000);
    QGeoCoordinate expected(50.0, 30.0, 100.0);
    expected = expected.atDistanceAndAzimuth(1.0, 0.0);
    expected = expected.atDistanceAndAzimuth(2.0, 90.0);
    CHECK(projected.time == sampleTime + 1000);
    CHECK(projected.alt == 99'500);
    CHECK(projected.mslAlt == 199'500);
    CHECK(near(projected.lat * 1.0e-7,
               expected.latitude(),
               1.1e-7));
    CHECK(near(projected.lon * 1.0e-7,
               expected.longitude(),
               1.1e-7));

    const QGeoCoordinate rawImu = scout.lastRawIMU();
    CHECK(near(rawImu.latitude(), 50.0, 1.0e-9));
    CHECK(near(rawImu.longitude(), 30.0, 1.0e-9));

    scout.setAttitude(bootTime,
                      0.1F,
                      6.274F,
                      -0.2F,
                      0.2F,
                      0.1F,
                      -0.3F);
    CHECK(scout.lastAttitudeTime() == sampleTime);
    const EAttitude projectedAttitude =
        scout.attitude(sampleTime + 100, false);
    CHECK(near(projectedAttitude.pitch,
               0.1 * 57.296 + 0.2 * 57.296 / 10.0,
               1.0e-4));
    CHECK(near(projectedAttitude.roll,
               -0.2 * 57.296 - 0.3 * 57.296 / 10.0,
               1.0e-4));
    CHECK(near(projectedAttitude.yaw,
               std::fmod(6.274 * 57.296 + 0.1 * 57.296 / 10.0,
                         360.0),
               1.0e-4));

    scout.setEPosition(sampleTime,
                       2,
                       8,
                       latitude + 10,
                       longitude + 20,
                       1234,
                       1.25F,
                       -2.5F,
                       0.5F,
                       200.0F,
                       361.0F);
    CHECK(scout.eLinkSatelliteStatus() == 2);
    CHECK(scout.eLinkSatelliteCount() == 8);
    const EPosition ePosition = scout.ePosition(sampleTime);
    CHECK(ePosition.type == 2);
    CHECK(ePosition.alt == 12'340);
    CHECK(ePosition.vx == 125);
    CHECK(ePosition.vy == -250);
    CHECK(ePosition.vz == 50);
    CHECK(ePosition.groundSpeed == 100.0F);
    CHECK(ePosition.groundCourse == 1.0F);

    scout.setGPSUsageMode(GPS_FORCE_USE);
    scout.setForceDisableGPS(true);
    CHECK(scout.currentState() == 2);
    CHECK(scout.lastGpsECoord().type == 2);

    RoadValidateResult road;
    road.time = sampleTime;
    road.accepted = true;
    road.offsetX = 2.0F;
    road.offsetY = 1.0F;
    scout.setRoadOffset(road);
    QGeoCoordinate shifted;
    scout.calculateTemporaryOffset(QGeoCoordinate(50.0, 30.0), shifted);
    CHECK(scout.hasRoadOffsets());
    CHECK(scout.lastRoadOffsetAccepted());
    CHECK(near(QGeoCoordinate(50.0, 30.0).distanceTo(shifted),
               std::sqrt(5.0),
               0.01));

    scout.setYawOffsetFromVNav(QDateTime::currentMSecsSinceEpoch(), 5.0F);
    CHECK(near(scout.currentYawOffset(), 5.0, 0.01));
    float vnavHeading = -1.0F;
    CHECK(!scout.vnavAzimuth(vnavHeading));
    CHECK(vnavHeading >= 0.0F);
}

void testRoadTypes()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    CHECK(sizeof(RoadPoint) == 0x20U);
    CHECK(sizeof(RoadItem) == 0x38U);
    CHECK(sizeof(RoadElement) == 0x80U);
    CHECK(sizeof(RoadMap) == 0x30U);
#endif

    RoadElement element;
    CHECK(element.index == -1);
    CHECK(element.type == 5);
    CHECK(element.enabled);
    CHECK(near(element.minimumConfidence, 0.51, 1.0e-7));
    element.addItem(-10.0F, {});
    CHECK(near(element.headings.at(0), 350.0, 1.0e-6));
    CHECK(near(element.itemLengths.at(0), 1000.0, 1.0e-5));
    CHECK(element.localItems.at(0).size() == 1);
    element.addItem(370.0F,
                    {QPointF(3.0, 4.0), QPointF(6.0, 8.0)});
    CHECK(near(element.headings.at(1), 10.0, 1.0e-6));
    CHECK(near(element.itemLengths.at(1), 10.0, 1.0e-5));

    const QGeoCoordinate start(50.0, 30.0);
    const QGeoCoordinate end = start.atDistanceAndAzimuth(100.0, 90.0);
    RoadMap map;
    map.fromCoords({start, end});
    CHECK(map.points.size() == 2);
    CHECK(map.items.size() == 1);
    CHECK(map.points.at(0).number == 1);
    CHECK(map.items.at(0).point0 == 0);
    CHECK(map.items.at(0).point1 == 1);
    CHECK(near(map.items.at(0).length, 100.0, 0.05));
    CHECK(near(map.items.at(0).heading, 90.0, 0.05));

    const QGeoCoordinate midpoint =
        start.atDistanceAndAzimuth(50.0, 90.0);
    const QGeoCoordinate observed =
        midpoint.atDistanceAndAzimuth(10.0, 0.0);
    QGeoCoordinate projected;
    RoadValidateOffset offset;
    int itemIndex = -1;
    const float distance =
        map.projCoord(observed, projected, offset, itemIndex);
    CHECK(itemIndex == 0);
    CHECK(near(distance, 10.0, 0.05));
    CHECK(near(offset.east, 0.0, 0.05));
    CHECK(near(offset.north, -10.0, 0.05));
    CHECK(projected.distanceTo(midpoint) < 0.06);

    const QGeoCoordinate newCenter =
        map.center.atDistanceAndAzimuth(1000.0, 0.0);
    CHECK(!map.checkNewCenter(newCenter, 1001.0F));
    CHECK(map.checkNewCenter(newCenter, 999.0F));
    CHECK(map.center.distanceTo(newCenter) < 0.001);

    map.clear();
    CHECK(map.points.isEmpty());
    CHECK(map.items.isEmpty());
    CHECK(!map.center.isValid());
}

void testThunderGaze()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    CHECK(sizeof(Target) == 0x58U);
#endif
    CHECK(sizeof(RoadPointAngle) == 0x10U);
    CHECK(sizeof(IgnoreBound) == 0x20U);

    const struct VersionCase {
        float version;
        qsizetype size;
        const char* className;
    } versions[] = {
        {1.0F, 0x330, "ThunderGazeV1"},
        {2.0F, 0xfe30, "ThunderGazeV2"},
        {3.0F, 0xfdf0, "ThunderGazeV3"},
        {4.0F, 0xfdf0, "ThunderGazeV4"},
        {5.0F, 0xfdec, "ThunderGazeV5"},
    };
    for (const VersionCase& item : versions) {
        std::unique_ptr<ThunderGaze> gaze(
            ThunderGazeFactory::create(item.version, nullptr, nullptr));
        CHECK(gaze != nullptr);
        CHECK(qstrcmp(gaze->metaObject()->className(), item.className) == 0);
        CHECK(gaze->messageSize() == item.size);
        CHECK(!gaze->parseMessage(QByteArray(item.size - 1, '\0'),
                                  0,
                                  QHostAddress(),
                                  0));
        CHECK(gaze->parseMessage(QByteArray(item.size, '\0'),
                                 0,
                                 QHostAddress(),
                                 0));
    }

    CameraT205 camera(QStringLiteral("/dev/null"));
    ThunderGazeV1 gaze(nullptr, nullptr);
    gaze.setCamera(&camera);
    gaze.setPTZOffset(1.0F, 2.0F);
    CameraParams params = camera.currentParams();
    params.zoom.widthAngle = 60.0F;
    params.zoom.heightAngle = 30.0F;
    QVector<RoadPointAngle> angles;
    gaze.calculateRoadPointAngles(params,
                                  {QPoint(960, 540), QPoint(0, 0)},
                                  angles);
    CHECK(angles.size() == 2);
    CHECK(near(angles.at(0).angleX, 1.0, 1.0e-6));
    CHECK(near(angles.at(0).angleY, 2.0, 1.0e-6));
    CHECK(near(angles.at(1).angleX, -29.0, 1.0e-6));
    CHECK(near(angles.at(1).angleY, 17.0, 1.0e-6));

    qlonglong partsTime = -1;
    int left = -1;
    int middle = -1;
    int right = -1;
    QObject::connect(&gaze,
                     &ThunderGaze::roadScreenParts,
                     [&](const qlonglong time,
                         const int leftValue,
                         const int middleValue,
                         const int rightValue) {
                         partsTime = time;
                         left = leftValue;
                         middle = middleValue;
                         right = rightValue;
                     });
    gaze.splitRoadScreenParts(123, {QPoint(0, 1),
                                    QPoint(700, 1),
                                    QPoint(1919, 1)});
    CHECK(partsTime == 123);
    CHECK(left == 1);
    CHECK(middle == 3); // Binary passes total count in the middle field.
    CHECK(right == 1);

    QVector<QPoint> reduced{QPoint(10, 10),
                            QPoint(11, 11),
                            QPoint(1000, 500)};
    gaze.reduceRoadPoints(reduced);
    CHECK(reduced.size() == 2);
}

void testVision()
{
    EVision vision(0, 5.0F);
    CHECK(vision.isConnected());
    CHECK(vision.isCalibrated());
    CHECK(vision.thunderGaze() != nullptr);
    CHECK(qstrcmp(vision.thunderGaze()->metaObject()->className(),
                  "ThunderGazeV5")
          == 0);

    const QMetaObject* const meta = vision.metaObject();
    CHECK(meta->indexOfSignal("targetDetected(CameraParams,QList<Target>)")
          >= 0);
    CHECK(meta->indexOfSignal(
              "roadDetected(qlonglong,CameraParams,QVector<RoadPointAngle>,bool)")
          >= 0);
    CHECK(meta->indexOfMethod("setGimbal(Gimbal)") >= 0);
    CHECK(meta->indexOfMethod("sendImageTarget(qlonglong,short,short,uchar)")
          >= 0);

    float horizontal = 999.0F;
    float vertical = 999.0F;
    vision.pointAngles(960, 540, horizontal, vertical);
    CHECK(near(horizontal, 0.0, 1.0e-4));
    CHECK(near(vertical, 0.0, 1.0e-4));
    vision.pointAngles(0, 0, horizontal, vertical);
    CHECK(near(horizontal, -64.16, 1.0e-3));
    CHECK(near(vertical, 49.93, 1.0e-3));

    vision.setCameraOffset(1.5F, -2.0F);
    vision.pointAngles(960, 540, horizontal, vertical);
    CHECK(near(horizontal, 1.5, 1.0e-4));
    CHECK(near(vertical, -2.0, 1.0e-4));

    int targetEvents = 0;
    QList<Target> received;
    QObject::connect(&vision,
                     &EVision::targetDetected,
                     [&](const CameraParams&, const QList<Target>& targets) {
                         ++targetEvents;
                         received = targets;
                     });
    vision.setTestTarget(960, 540);
    CHECK(targetEvents == 1);
    CHECK(received.size() == 1);
    CHECK(received.first().pixelX == 960);
    CHECK(received.first().pixelY == 540);
    CHECK(received.first().type == 3);
    CHECK(near(received.first().angleX, 1.5, 1.0e-4));
    CHECK(near(received.first().angleY, -2.0, 1.0e-4));

    int rtspOn = 0;
    int rtspOff = 0;
    QObject::connect(&vision, &EVision::rtspEnabled, [&] { ++rtspOn; });
    QObject::connect(&vision, &EVision::rtspDisabled, [&] { ++rtspOff; });
    vision.enableRTSP();
    vision.disableRTSP();
    CHECK(rtspOn == 1);
    CHECK(rtspOff == 1);

    vision.setUSBCamera(QStringLiteral("/dev/null"));
    CHECK(vision.camera() != nullptr);
    CHECK(near(vision.cameraCalibrationWidthAngle(), 9.0, 1.0e-5));
    CHECK(near(vision.cameraCalibrationHeightAngle(), 5.0, 1.0e-5));
}

void testRoadAnalysisAndRunner()
{
    const QGeoCoordinate center(50.0, 30.0);
    RoadMap junction;
    junction.points = {
        RoadPoint{0, -1, center, QPointF(0.0, 0.0)},
        RoadPoint{1,
                  -1,
                  center.atDistanceAndAzimuth(100.0, 0.0),
                  QPointF(0.0, 100.0)},
        RoadPoint{2,
                  -1,
                  center.atDistanceAndAzimuth(100.0, 90.0),
                  QPointF(100.0, 0.0)},
        RoadPoint{3,
                  -1,
                  center.atDistanceAndAzimuth(100.0, 270.0),
                  QPointF(-100.0, 0.0)},
    };
    for (int index = 0; index < 3; ++index) {
        RoadItem item;
        item.index = index;
        item.point0 = 0;
        item.point1 = index + 1;
        junction.items.append(item);
    }
    junction.center = center;

    RoadAnalyzerV2 analyzer;
    analyzer.setMap(&junction);
    QVector<RoadElement> analyzed;
    QObject::connect(&analyzer,
                     &RoadAnalyzer::elementsDetected,
                     [&](const QVector<RoadElement>& elements) {
                         analyzed = elements;
                     });
    analyzer.analyzeMap();
    CHECK(!analyzed.isEmpty());
    CHECK(analyzed.first().isFork);
    CHECK(analyzed.first().localItems.size() == 3);
    CHECK(analyzer.localPoints().size() == 4);

    RoadNetworkAnalyzer network;
    QSet<int> selected;
    const float dispersion = network.dispersion(
        {QPointF(3.0, 4.0), QPointF(6.0, 8.0), QPointF(30.0, 40.0)},
        QPointF(),
        selected,
        11.0F);
    CHECK(selected.size() == 2);
    CHECK(near(dispersion, std::sqrt(62.5), 1.0e-5));
    CHECK(network.isUniqueDotPattern(
        {QPointF(1.0, 2.0), QPointF(3.0, 4.0)}));
    CHECK(!network.isUniqueDotPattern(
        {QPointF(1.0, 2.0), QPointF(1.0, 2.0)}));

    RoadMap line;
    const QGeoCoordinate start(50.0, 30.0);
    const QGeoCoordinate end = start.atDistanceAndAzimuth(100.0, 90.0);
    line.fromCoords({start, end});
    RoadRunner runner;
    runner.setMap(&line);
    runner.setEnabled(true);
    CHECK(runner.isEnabled());
    CHECK(runner.isAlongOffsetEnabled());
    CHECK(runner.isCalculatePlaneYawEnabled());
    CHECK(runner.isSearchRoadElementsEnabled());
    CHECK(runner.currentMode() == RoadRunner::DisabledMode);
    runner.setMode(RoadRunner::RoadMode);
    CHECK(runner.currentMode() == RoadRunner::RoadMode);

    float movedEast = 1.0F;
    float movedNorth = 1.0F;
    runner.movePlaneAlongRoad(start, 0, 50.0F, movedEast, movedNorth);
    CHECK(movedEast == 0.0F);
    CHECK(movedNorth == 0.0F);
    CHECK(runner.isEqualOffsets(0.0, 0.0, 100.0, 100.0));
    CHECK(!runner.isEqualOffsets(0.0, 0.0, 200.0, 0.0));

    const QGeoCoordinate midpoint =
        start.atDistanceAndAzimuth(50.0, 90.0);
    const QGeoCoordinate observed =
        midpoint.atDistanceAndAzimuth(10.0, 0.0);
    CHECK(near(runner.roadDistance(observed), 10.0, 0.05));
    int added = 0;
    int changed = 0;
    RoadValidateResult last;
    QObject::connect(&runner,
                     &RoadRunner::offsetItemAdded,
                     [&](const RoadValidateResult& result) {
                         ++added;
                         last = result;
                     });
    QObject::connect(&runner,
                     &RoadRunner::offsetChanged,
                     [&](const RoadValidateResult& result) {
                         ++changed;
                         last = result;
                     });
    runner.addData({},
                   true,
                   observed,
                   observed,
                   midpoint,
                   1,
                   2,
                   start,
                   end,
                   100,
                   90.0F,
                   30.0F,
                   100.0F,
                   0.0F,
                   100.0F,
                   true);
    CHECK(added == 1);
    CHECK(changed == 1);
    CHECK(last.roadItem == 0);
    CHECK(last.accepted);
    CHECK(near(last.offset, 10.0, 0.05));
    CHECK(near(last.offsetX, 0.0, 0.05));
    CHECK(near(last.offsetY, -10.0, 0.05));
    CHECK(runner.lastDetectedTimeRoadItem(1) > 0);

    QGeoCoordinate shifted;
    CHECK(runner.calculateTemporaryOffset(observed, shifted));
    CHECK(shifted.distanceTo(midpoint) < 0.1);
    runner.accept();
    CHECK(changed == 2);
    runner.setEnabled(false);
    CHECK(!runner.isEnabled());
    CHECK(!runner.calculateTemporaryOffset(observed, shifted));

    const QMetaObject* const meta = runner.metaObject();
    CHECK(meta->indexOfSignal("offsetItemAdded(RoadValidateResult)") >= 0);
    CHECK(meta->indexOfSignal("lookAt(QGeoCoordinate)") >= 0);
    CHECK(meta->indexOfMethod("detectedElementsHandler(QVector<RoadElement>)")
          >= 0);
}

void testPathfinderFacade()
{
    CHECK(mode2string(PathfinderMode::Disabled) == QStringLiteral("DISABLED"));
    CHECK(mode2string(PathfinderMode::TargetPositionCorrection)
          == QStringLiteral("TRG_POS_CORRECTION"));
    CHECK(mode2string(PathfinderMode::Parade) == QStringLiteral("PARADE"));
    CHECK(mode2string(static_cast<PathfinderMode>(22))
          == QStringLiteral("UNKNOWN"));

    EPathfinderParams params;
    params.setVehicleVersion(17);
    CHECK(params.vehicleVersion() == 17);
    EScout scout;
    EPathfinder pathfinder(&scout, params);
    CHECK(pathfinder.currentMode() == PathfinderMode::Disabled);
    CHECK(pathfinder.currentLaunchMode() == LaunchMode::Mode1);
    CHECK(pathfinder.burnRocketDelay() == 30);
    CHECK(pathfinder.vehicleRole() == 2);

    int requestedMode = -1;
    QObject::connect(&pathfinder, &EPathfinder::setMode,
                     [&](const int mode) { requestedMode = mode; });
    pathfinder.setCurrentMode(PathfinderMode::FindLine);
    CHECK(pathfinder.currentMode() == PathfinderMode::FindLine);
    CHECK(requestedMode == 5);

    pathfinder.airspeedEvent(10.0F, 8.0F);
    CHECK(pathfinder.airSpeedLastValue() == 36.0F);
    pathfinder.currentRcValues(1005, 1006, 1007, 1008, 1009, 1010, 1011,
                               1012, 1013, 1014, 1015, 1016, 1017, 1018);
    CHECK(pathfinder.rcValue(4) == 0);
    CHECK(pathfinder.rcValue(5) == 1005);
    CHECK(pathfinder.rcValue(18) == 1018);
    CHECK(pathfinder.rcValue(19) == 0);

    pathfinder.setExtremeGimbalTest(true);
    CHECK(pathfinder.extremeGimbalTestIsEnabled());
    pathfinder.setVehicleVersion(16);
    CHECK(pathfinder.vehicleRole() == 1);
    pathfinder.setVehicleVersion(15);
    CHECK(pathfinder.vehicleRole() == 0);

    const QMetaObject* const meta = pathfinder.metaObject();
    CHECK(meta->methodCount() - meta->methodOffset() == 170);
    CHECK(meta->indexOfSignal("controlOn(int)") >= 0);
    CHECK(meta->indexOfSignal("roadData(QVector<QGeoCoordinate>,bool,QGeoCoordinate,QGeoCoordinate,QGeoCoordinate,int,int,QGeoCoordinate,QGeoCoordinate,int,float,float,float,float,float,bool,bool)") >= 0);
    CHECK(meta->indexOfMethod("setRadarPlaneTarget(PlaneTarget)") >= 0);
    CHECK(meta->indexOfMethod("vehicleDetected(qlonglong,QList<VehicleTarget>)") >= 0);
}

void testControllers()
{
    ClientController client;
    int statusRequests = 0;
    int parameterRequests = 0;
    int targetStops = 0;
    QObject::connect(&client, &ClientController::getStatusMessage,
                     [&](const QJsonDocument& message) {
                         ++statusRequests;
                         CHECK(message.object().value(QStringLiteral("cookie"))
                               == QStringLiteral("a"));
                     });
    QObject::connect(&client, &ClientController::setParamsMessage,
                     [&](const QJsonDocument&) { ++parameterRequests; });
    QObject::connect(&client, &ClientController::stopCurrentTarget,
                     [&] { ++targetStops; });

    QByteArray requests =
        QByteArrayLiteral("{\"cookie\":\"a\",\"messageType\":2}")
        + QByteArrayLiteral("{\"cookie\":\"b\",\"messageType\":8,"
                            "\"params\":{\"vehicleVersion\":17}}")
        + QByteArrayLiteral("{\"cookie\":\"c\",\"messageType\":800,"
                            "\"exampleId\":-1}");
    ClientControllerTestProbe::parse(client, requests);
    CHECK(requests.isEmpty());
    CHECK(statusRequests == 1);
    CHECK(parameterRequests == 1);
    CHECK(targetStops == 1);

    VehicleController vehicle;
    int answers = 0;
    int statusChanges = 0;
    QJsonDocument lastAnswer;
    QObject::connect(&vehicle, &VehicleController::sendAnswer,
                     [&](const QJsonDocument& answer) {
                         ++answers;
                         lastAnswer = answer;
                     });
    QObject::connect(&vehicle, &VehicleController::updateVehicleStatus,
                     [&](const VehicleStatus&) { ++statusChanges; });
    vehicle.Init();
    CHECK(vehicle.scout() != nullptr);
    CHECK(vehicle.pathfinder() != nullptr);
    CHECK(statusChanges == 1);

    QJsonObject request{{QStringLiteral("cookie"), QStringLiteral("state")}};
    vehicle.getStatusMessage(QJsonDocument(request));
    CHECK(answers == 1);
    CHECK(lastAnswer.object().value(QStringLiteral("cookie"))
          == QStringLiteral("state"));
    CHECK(lastAnswer.object().value(QStringLiteral("result")).toInt() == 0);

    vehicle.arm(true);
    CHECK(vehicle.status().armed);
    vehicle.setMode(5);
    CHECK(vehicle.pathfinder()->currentMode() == PathfinderMode::FindLine);
    vehicle.launchMessage(QJsonDocument(request));
    CHECK(vehicle.status().launched);

    const QMetaObject* const clientMeta = client.metaObject();
    CHECK(clientMeta->methodCount() - clientMeta->methodOffset() == 56);
    CHECK(clientMeta->indexOfSignal("setNodesMessage(QJsonDocument)") >= 0);
    CHECK(clientMeta->indexOfMethod("parseBuffer(QByteArray&,QTcpSocket*)") >= 0);
    const QMetaObject* const vehicleMeta = vehicle.metaObject();
    CHECK(vehicleMeta->methodCount() - vehicleMeta->methodOffset() == 97);
    CHECK(vehicleMeta->indexOfSignal("updateVehicleStatus(VehicleStatus)") >= 0);
    CHECK(vehicleMeta->indexOfMethod("getStatusMessage(QJsonDocument)") >= 0);
    CHECK(vehicleMeta->indexOfMethod("vnavMapCRCProcessReadOutput()") >= 0);
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    testCrc();
    testAngles();
    testIntersection();
    testKalman();
    testPixelMapper();
    testGimbalRoundTrip();
    testCameraRecovery();
    testLivoxVersion();
    testLivoxDataPath();
    testVehicleLed();
    testMavlinkDialect();
#ifdef EPATHFINDER_HAS_MAVLINK
    testMavlinkCommunicator();
    testMavlinkPlanHandler();
    testMavlinkHandlers();
    testMavlinkControlHandler();
    testMavlinkLogHandler();
#endif
    testELinkCore();
    testELinkFoundationalHandlers();
    testELinkStatusAndInterceptorHandlers();
    testELinkPositionHandler();
    testELinkFlowerHandler();
    testELinkGimbalHandler();
    testELinkShumodavHandler();
#ifdef EPATHFINDER_HAS_MAVLINK
    testELinkTelemetry();
#endif
    testVNav();
    testNetworkLinks();
    testApplicationSupportAndInterception();
    testScout();
    testRoadTypes();
    testThunderGaze();
    testVision();
    testRoadAnalysisAndRunner();
    testPathfinderFacade();
    testControllers();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
