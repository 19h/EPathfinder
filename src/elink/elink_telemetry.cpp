#include "elink/elink_telemetry.hpp"

#include "core/crc.hpp"
#include "elink/elink_communicator.hpp"
#include "mavlink/plan_handler.hpp"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTimer>
#include <QTimerEvent>
#include <QUdpSocket>

#include <algorithm>
#include <cstring>
#include <limits>
#include <type_traits>

namespace {

template <typename T>
void appendIntegral(QByteArray& output, T value)
{
    static_assert(std::is_integral_v<T>);
    using Unsigned = std::make_unsigned_t<T>;
    const Unsigned unsignedValue = static_cast<Unsigned>(value);
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        output.append(static_cast<char>(
            (unsignedValue >> (8U * i)) & static_cast<Unsigned>(0xffU)));
    }
}

void appendFloat(QByteArray& output, const float value)
{
    std::uint32_t bits{};
    static_assert(sizeof(bits) == sizeof(value));
    std::memcpy(&bits, &value, sizeof(bits));
    appendIntegral(output, bits);
}

template <typename T>
bool readIntegral(const QByteArray& input, const qsizetype offset, T& value)
{
    static_assert(std::is_integral_v<T>);
    if (offset < 0
        || input.size() - offset < static_cast<qsizetype>(sizeof(T))) {
        return false;
    }
    using Unsigned = std::make_unsigned_t<T>;
    Unsigned result{};
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        result |= static_cast<Unsigned>(
                      static_cast<unsigned char>(input[offset + i]))
            << (8U * i);
    }
    value = static_cast<T>(result);
    return true;
}

float readRawFloat(const char* const data)
{
    std::uint32_t bits{};
    for (std::size_t i = 0; i < sizeof(bits); ++i) {
        bits |= static_cast<std::uint32_t>(
                    static_cast<unsigned char>(data[i]))
            << (8U * i);
    }
    float value{};
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

template <typename T>
T readRawIntegral(const char* const data)
{
    static_assert(std::is_integral_v<T>);
    using Unsigned = std::make_unsigned_t<T>;
    Unsigned result{};
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        result |= static_cast<Unsigned>(
                      static_cast<unsigned char>(data[i]))
            << (8U * i);
    }
    return static_cast<T>(result);
}

QByteArray telemetryPrefix(const unsigned short planeId,
                           const unsigned char type)
{
    QByteArray result;
    result.reserve(13);
    result.append(static_cast<char>(0xe0));
    result.append(static_cast<char>(0x0e));
    appendIntegral(result, planeId);
    result.append(static_cast<char>(type));
    return result;
}

constexpr unsigned char kRadarType = 0x13U;

} // namespace

ELinkTelemetry::ELinkTelemetry(ELinkCommunicator* const communicator,
                               const unsigned char configuration)
    : ELinkAbstractHandler(communicator)
    , configuration_(configuration)
    , pingProcess_(new QProcess(this))
{
    connect(pingProcess_,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &ELinkTelemetry::pingTelemetryFinished);
    QTimer::singleShot(500, this, &ELinkTelemetry::pingTelemetry);
    startTimer(directUdpMode_ ? 20 : 50, Qt::CoarseTimer);
}

ELinkTelemetry::~ELinkTelemetry()
{
    delete telemetrySocket_;
    delete telemetrySocket2_;
    delete commandReservationSocket_;
    delete modemCommandSocket_;
    delete epLaunchSocket_;
}

void ELinkTelemetry::setVersion(const unsigned char version)
{
    version_ = version;
}

unsigned short ELinkTelemetry::telemetryPlaneId() const
{
    return planeId_;
}

void ELinkTelemetry::setTelemetryPlaneId(const unsigned short planeId)
{
    planeId_ = planeId;
}

QString ELinkTelemetry::telemetryPlaneUid() const
{
    return planeUid_;
}

void ELinkTelemetry::setTelemetryPlaneUid(const QString& uid)
{
    planeUid_ = uid;
}

bool ELinkTelemetry::isConnected() const
{
    return connected_;
}

void ELinkTelemetry::getTelemetryStatus()
{
    sendProtocolMessage(0x8bU, QByteArray(1, static_cast<char>(1)));
}

void ELinkTelemetry::setTelemetryConfig(const unsigned char first,
                                        const unsigned char second)
{
    QByteArray body;
    body.append(static_cast<char>(0));
    body.append(static_cast<char>(first));
    body.append(static_cast<char>(second));
    sendProtocolMessage(0x8bU, body);
}

void ELinkTelemetry::replaceSocket(QUdpSocket*& socket)
{
    if (socket == nullptr) {
        return;
    }
    socket->disconnect(this);
    delete socket;
    socket = nullptr;
}

void ELinkTelemetry::connectToUDPTelemetry()
{
    replaceSocket(telemetrySocket_);
    replaceSocket(telemetrySocket2_);

    telemetrySocket_ = new QUdpSocket;
    telemetrySocket_->bind(QHostAddress(hostAddress_), telemetryPort_);
    connect(telemetrySocket_,
            &QUdpSocket::readyRead,
            this,
            &ELinkTelemetry::readTelemetryPendingDatagrams);

    telemetrySocket2_ = new QUdpSocket;
    telemetrySocket2_->bind(QHostAddress(hostAddress_), telemetryPort2_);
    connect(telemetrySocket2_,
            &QUdpSocket::readyRead,
            this,
            &ELinkTelemetry::readTelemetryPendingDatagrams2);
}

void ELinkTelemetry::setHostAddress(const QString& address)
{
    hostAddress_ = address;
    connectToUDPTelemetry();
}

void ELinkTelemetry::connectToUDPCommands()
{
    replaceSocket(commandReservationSocket_);
    commandReservationSocket_ = new QUdpSocket;
    commandReservationSocket_->bind(telemetryPort_);
    commandReservationSocket_->deleteLater();
    commandReservationSocket_ = nullptr;

    replaceSocket(modemCommandSocket_);
    modemCommandSocket_ = new QUdpSocket;
    modemCommandSocket_->bind(12012U);
    connect(modemCommandSocket_,
            &QUdpSocket::readyRead,
            this,
            &ELinkTelemetry::readModemCommandPendingDatagrams);
}

void ELinkTelemetry::connectToUDPEPLaunch()
{
    replaceSocket(epLaunchSocket_);
    epLaunchSocket_ = new QUdpSocket(this);
    epLaunchSocket_->bind(epLaunchReceivePort_);
    connect(epLaunchSocket_,
            &QUdpSocket::readyRead,
            this,
            &ELinkTelemetry::readEPLaunchDatagrams);
}

void ELinkTelemetry::Init(const QString& address, const unsigned short port)
{
    hostAddress_ = address;
    telemetryPort_ = port;
    telemetryPort2_ = static_cast<unsigned short>(port + 1U);
    connectToUDPTelemetry();
    connectToUDPCommands();
    connectToUDPEPLaunch();
    if (!directUdpMode_) {
        setTelemetryConfig(0U, 0U);
    }
}

void ELinkTelemetry::addPlaneId2(QByteArray& data) const
{
    if (planeSid_ != 0U) {
        data.append(static_cast<char>(6));
        appendIntegral(data, planeSid_);
    } else {
        const QByteArray uid = planeUid_.toLocal8Bit();
        data.append(static_cast<char>(5));
        data.append(static_cast<char>(uid.size()));
        data.append(uid);
    }
    appendIntegral(data, planeId_);
}

void ELinkTelemetry::addMessageInQueue(const unsigned char type,
                                       const QByteArray& data)
{
    // The LoRa mode accepts only radar data.  Direct UDP mode accepts all
    // types.  This is the exact branch at RVA 0x3d930.
    if (!directUdpMode_ && type != kRadarType) {
        return;
    }

    const auto iterator = std::find_if(
        messages_.begin(), messages_.end(),
        [type](const TelemetryMessage& message) {
            return message.type == type;
        });
    if (iterator != messages_.end()) {
        iterator->data = data;
        iterator->id = telemetryMessageId_;
        return;
    }

    messages_.append(TelemetryMessage{type, data, telemetryMessageId_});
    if (type == kRadarType) {
        ++telemetryMessageId_;
    }
}

void ELinkTelemetry::sendPosAtt2(float, float, float, float)
{
    // RVA 0x3abc0 is a literal return instruction.
}

void ELinkTelemetry::sendPosAtt(qlonglong time,
                                const int lat,
                                const int lon,
                                const int alt,
                                const float roll,
                                const float pitch,
                                const float yaw,
                                const float airSpeed)
{
    if (version_ != 0U) {
        sendPosAtt2(roll, pitch, yaw, airSpeed);
    }
    if (planeId_ == 0U) {
        return;
    }
    if (time == 0) {
        time = QDateTime::currentMSecsSinceEpoch();
    }
    QByteArray data = telemetryPrefix(planeId_, 0x01U);
    appendIntegral(data, time);
    appendIntegral(data, lat);
    appendIntegral(data, lon);
    appendIntegral(data, alt);
    appendFloat(data, roll);
    appendFloat(data, pitch);
    appendFloat(data, yaw);
    appendFloat(data, airSpeed);
    addMessageInQueue(0x01U, data);
}

void ELinkTelemetry::sendGPSPos2(const qlonglong time,
                                 const int lat,
                                 const int lon,
                                 const int alt)
{
    if (telemetrySocket2_ == nullptr || planeUid_.isEmpty()) {
        return;
    }
    QByteArray data;
    data.append(static_cast<char>(0xe0));
    data.append(static_cast<char>(0x0e));
    addPlaneId2(data);
    data.append(static_cast<char>(0x11));
    appendIntegral(data, time);
    appendIntegral(data, lat);
    appendIntegral(data, lon);
    appendIntegral(data, alt);
    telemetrySocket2_->writeDatagram(
        data, QHostAddress(hostAddress_), telemetryPort2_);
}

void ELinkTelemetry::sendGPSPos(qlonglong time,
                                const int lat,
                                const int lon,
                                const int alt)
{
    if (version_ != 0U) {
        sendGPSPos2(time, lat, lon, alt);
    }
    if (planeId_ == 0U) {
        return;
    }
    if (time == 0) {
        time = QDateTime::currentMSecsSinceEpoch();
    }
    QByteArray data = telemetryPrefix(planeId_, 0x11U);
    appendIntegral(data, time);
    appendIntegral(data, lat);
    appendIntegral(data, lon);
    appendIntegral(data, alt);
    addMessageInQueue(0x11U, data);
}

void ELinkTelemetry::sendTelemetryCameraParams(
    const qlonglong time,
    const float gimbalRoll,
    const float gimbalPitch,
    const float gimbalYaw)
{
    if (telemetrySocket2_ == nullptr || planeUid_.isEmpty()) {
        return;
    }
    QByteArray data;
    data.append(static_cast<char>(0xe0));
    data.append(static_cast<char>(0x0e));
    addPlaneId2(data);
    data.append(static_cast<char>(0x15));
    appendIntegral(data, time);
    data.append(static_cast<char>(static_cast<int>(gimbalRoll)));
    data.append(static_cast<char>(static_cast<int>(gimbalPitch)));
    data.append(static_cast<char>(static_cast<int>(gimbalYaw)));
    telemetrySocket2_->writeDatagram(
        data, QHostAddress(hostAddress_), telemetryPort2_);
}

void ELinkTelemetry::sendLastWPPos(qlonglong time,
                                   const int lat,
                                   const int lon)
{
    if (planeId_ == 0U) {
        return;
    }
    if (time == 0) {
        time = QDateTime::currentMSecsSinceEpoch();
    }
    QByteArray data = telemetryPrefix(planeId_, 0x14U);
    appendIntegral(data, time);
    appendIntegral(data, lat);
    appendIntegral(data, lon);
    addMessageInQueue(0x14U, data);
}

void ELinkTelemetry::sendRadarEmulationInfo(qlonglong time,
                                            const int lat,
                                            const int lon,
                                            const int alt,
                                            const float azimuth,
                                            const float groundSpeed)
{
    if (planeId_ == 0U) {
        return;
    }
    if (time == 0) {
        time = QDateTime::currentMSecsSinceEpoch();
    }

    QByteArray inner = telemetryPrefix(planeId_, kRadarType);
    appendIntegral(inner, time);
    appendIntegral(inner, lat);
    appendIntegral(inner, lon);
    appendIntegral(inner, alt);
    appendFloat(inner, azimuth);
    appendFloat(inner, groundSpeed);
    appendIntegral(inner, telemetryMessageId_);

    if (directUdpMode_) {
        addMessageInQueue(kRadarType, inner);
        return;
    }

    // The binary's LoRa wrapper has a one-byte length/CRC inconsistency:
    // it declares 38 bytes, stores 39, and computes CRC over all but the high
    // byte of the inner message id.  Preserve those exact bytes.
    QByteArray frame;
    frame.reserve(43);
    frame.append(static_cast<char>(0xd0));
    frame.append(static_cast<char>(0x0d));
    appendIntegral(frame, static_cast<std::uint16_t>(38U));
    const qsizetype checksumStart = frame.size();
    frame.append(static_cast<char>(sequence_++));
    frame.append(static_cast<char>(0x8a));
    frame.append(inner);
    const std::uint16_t checksum = crc16(
        reinterpret_cast<const std::uint8_t*>(
            frame.constData() + checksumStart),
        36U);
    appendIntegral(frame, checksum);
    addMessageInQueue(kRadarType, frame);
}

void ELinkTelemetry::sendPlan2(qlonglong, const Plan&)
{
    // RVA 0x3b268 builds only a temporary header and never writes it.
}

void ELinkTelemetry::sendPlan(const qlonglong time, const Plan plan)
{
    if (!plan.items.isEmpty()) {
        planPresent_ = true;
    }
    if (version_ != 0U) {
        sendPlan2(time, plan);
    }
    if (planeId_ == 0U) {
        return;
    }

    QJsonArray nodes;
    for (int index = 1; index < plan.items.size(); ++index) {
        const PlanItem& item = plan.items.at(index);
        QJsonObject object;
        object.insert(QStringLiteral("lat"), item.latitude);
        object.insert(QStringLiteral("lon"), item.longitude);
        object.insert(QStringLiteral("alt"), item.altitude);
        object.insert(QStringLiteral("type"),
                      static_cast<int>(item.type));
        if (item.type == PlanItemType::Search) {
            object.insert(QStringLiteral("radius"), item.radius);
            object.insert(QStringLiteral("timeout"), item.timeout);
        }
        object.insert(QStringLiteral("road_type"),
                      static_cast<int>(item.roadType));
        object.insert(QStringLiteral("trust_gps"), item.trustGps);
        nodes.append(object);
    }
    QJsonObject root;
    root.insert(QStringLiteral("nodes"), nodes);
    const QByteArray json = QJsonDocument(root).toJson();

    QByteArray data = telemetryPrefix(planeId_, 0x12U);
    appendIntegral(data, time);
    appendIntegral(data, static_cast<std::uint16_t>(json.size()));
    data.append(json);
    addMessageInQueue(0x12U, data);
}

void ELinkTelemetry::sendGPSStatus(const qlonglong time,
                                   const unsigned char epStatus,
                                   const unsigned char gpsStatus,
                                   const unsigned char satellites)
{
    if (planeId_ == 0U) {
        return;
    }
    QByteArray data = telemetryPrefix(planeId_, 0x02U);
    appendIntegral(data, time);
    data.append(static_cast<char>(epStatus));
    data.append(static_cast<char>(gpsStatus));
    data.append(static_cast<char>(satellites));
    addMessageInQueue(0x02U, data);
}

void ELinkTelemetry::sendCurrentFlightMode(const qlonglong time,
                                           const unsigned char mode)
{
    if (planeId_ == 0U) {
        return;
    }
    QByteArray data = telemetryPrefix(planeId_, 0x21U);
    appendIntegral(data, time);
    data.append(static_cast<char>(mode));
    addMessageInQueue(0x21U, data);
}

void ELinkTelemetry::sendCurrentMode2(qlonglong,
                                      const unsigned char mode)
{
    if (telemetrySocket2_ == nullptr || planeUid_.isEmpty()) {
        return;
    }
    QByteArray data;
    data.append(static_cast<char>(0xe0));
    data.append(static_cast<char>(0x0e));
    addPlaneId2(data);
    data.append(static_cast<char>(0x03));
    data.append(static_cast<char>(mode));
    telemetrySocket2_->writeDatagram(
        data, QHostAddress(hostAddress_), telemetryPort2_);
}

void ELinkTelemetry::sendCurrentMode(const qlonglong time,
                                     const unsigned char mode,
                                     const unsigned short targetAltitude)
{
    Q_UNUSED(targetAltitude)
    currentMode_ = mode;
    if (version_ != 0U) {
        sendCurrentMode2(time, mode);
    }
    if (planeId_ == 0U) {
        return;
    }
    QByteArray data = telemetryPrefix(planeId_, 0x03U);
    appendIntegral(data, time);
    data.append(static_cast<char>(mode));
    addMessageInQueue(0x03U, data);
}

void ELinkTelemetry::sendPlanModeInfo(
    const qlonglong time,
    const unsigned char nextWaypoint,
    const unsigned short nextWaypointDistance,
    const float deltaAzimuth)
{
    if (planeId_ == 0U) {
        return;
    }
    QByteArray data = telemetryPrefix(planeId_, 0x34U);
    appendIntegral(data, time);
    data.append(static_cast<char>(nextWaypoint));
    appendIntegral(data, nextWaypointDistance);
    appendFloat(data, deltaAzimuth);
    addMessageInQueue(0x34U, data);
}

void ELinkTelemetry::sendFindModeInfo(const qlonglong time,
                                      const unsigned char direction,
                                      const unsigned short centerDistance)
{
    if (planeId_ == 0U) {
        return;
    }
    QByteArray data = telemetryPrefix(planeId_, 0x36U);
    appendIntegral(data, time);
    data.append(static_cast<char>(direction));
    appendIntegral(data, centerDistance);
    addMessageInQueue(0x36U, data);
}

void ELinkTelemetry::sendTelemetryDistanceInfo(
    const qlonglong time,
    const int battery,
    const int homeDistance,
    const int distanceTraveled)
{
    if (planeId_ == 0U) {
        return;
    }
    QByteArray data = telemetryPrefix(planeId_, 0x39U);
    appendIntegral(data, time);
    appendIntegral(data, battery);
    appendIntegral(data, homeDistance);
    appendIntegral(data, distanceTraveled);
    addMessageInQueue(0x39U, data);
}

void ELinkTelemetry::sendTargetCorrectionModeInfo(
    const qlonglong time,
    const int targetLat,
    const int targetLon,
    const unsigned short dtpCount,
    const unsigned short distance,
    const float deltaAzimuth)
{
    if (planeId_ == 0U) {
        return;
    }
    QByteArray data = telemetryPrefix(planeId_, 0x37U);
    appendIntegral(data, time);
    appendIntegral(data, targetLat);
    appendIntegral(data, targetLon);
    appendIntegral(data, dtpCount);
    appendIntegral(data, distance);
    appendFloat(data, deltaAzimuth);
    addMessageInQueue(0x37U, data);
}

void ELinkTelemetry::sendTargetModeInfo(
    const qlonglong time,
    const int targetLat,
    const int targetLon,
    const unsigned char state,
    const unsigned short dtpCount,
    const unsigned short distance,
    const float deltaAzimuth)
{
    if (planeId_ == 0U) {
        return;
    }
    QByteArray data = telemetryPrefix(planeId_, 0x38U);
    appendIntegral(data, time);
    appendIntegral(data, targetLat);
    appendIntegral(data, targetLon);
    data.append(static_cast<char>(state));
    appendIntegral(data, dtpCount);
    appendIntegral(data, distance);
    appendFloat(data, deltaAzimuth);
    addMessageInQueue(0x38U, data);
}

void ELinkTelemetry::sendExplorationInfo(
    const qlonglong time,
    const unsigned short realWindDirection,
    const float realWindSpeed)
{
    if (planeId_ == 0U) {
        return;
    }
    QByteArray data = telemetryPrefix(planeId_, 0x04U);
    appendIntegral(data, time);
    appendIntegral(data, realWindDirection);
    appendFloat(data, realWindSpeed);
    addMessageInQueue(0x04U, data);
}

void ELinkTelemetry::setLaunchReady(const bool ready)
{
    launchReady_ = ready;
}

void ELinkTelemetry::setLaunched()
{
    launched_ = true;
}

void ELinkTelemetry::sendModemCommand(const QByteArray& data)
{
    if (modemCommandSocket_ == nullptr) {
        return;
    }
    modemCommandSocket_->writeDatagram(
        data, QHostAddress(hostAddress_), telemetryPort_);
}

void ELinkTelemetry::getVideoStreamStatus()
{
    sendModemCommand(QByteArray::fromHex("1150D00002"));
}

void ELinkTelemetry::enableVideoStream()
{
    sendModemCommand(QByteArray::fromHex("1150D00001"));
}

void ELinkTelemetry::disableVideoStream()
{
    sendModemCommand(QByteArray::fromHex("1150D00000"));
}

void ELinkTelemetry::enableFullPower()
{
    sendModemCommand(QByteArray::fromHex("115093001400"));
    QTimer::singleShot(30000, this, &ELinkTelemetry::enableFullPower);
}

void ELinkTelemetry::startVideoStream()
{
    if (videoStreamStatus_ == 1U) {
        return;
    }
    enableVideoStream();
    QTimer::singleShot(30000, this, &ELinkTelemetry::getVideoStreamStatus);
    QTimer::singleShot(40000, this, &ELinkTelemetry::getVideoStreamStatus);
}

void ELinkTelemetry::readModemCommandPendingDatagrams()
{
    if (modemCommandSocket_ == nullptr) {
        return;
    }
    const QByteArray prefix = QByteArray::fromHex("1190D00002");
    while (modemCommandSocket_->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<qsizetype>(
            modemCommandSocket_->pendingDatagramSize()));
        modemCommandSocket_->readDatagram(datagram.data(), datagram.size());
        if (datagram.size() == 6 && datagram.startsWith(prefix)) {
            videoStreamStatus_ = static_cast<unsigned char>(datagram.at(5));
        }
    }
}

void ELinkTelemetry::emitTargetFromRetainedELinkMessage(
    const bool includeKinematics)
{
    if (messageData_ == nullptr || messageSize_ < 34U) {
        return;
    }
    const std::int32_t lat = readRawIntegral<std::int32_t>(messageData_ + 14);
    const std::int32_t lon = readRawIntegral<std::int32_t>(messageData_ + 18);
    const std::int32_t alt = readRawIntegral<std::int32_t>(messageData_ + 22);
    if (lat == lastTargetLat_ && lon == lastTargetLon_) {
        return;
    }
    lastTargetLat_ = lat;
    lastTargetLon_ = lon;

    PlaneTarget target;
    target.time = QDateTime::currentMSecsSinceEpoch() - 200;
    target.coordinate = QGeoCoordinate(
        static_cast<double>(lat) / 10000000.0,
        static_cast<double>(lon) / 10000000.0,
        static_cast<double>(alt) / 1000.0);
    if (includeKinematics) {
        target.azimuth = readRawFloat(messageData_ + 26);
        target.groundSpeed = readRawFloat(messageData_ + 30);
    }
    emit planeTarget(target);
}

void ELinkTelemetry::processMessage()
{
    if (messageType_ == 0x8bU) {
        if (directUdpMode_ || messageSize_ < 4U) {
            return;
        }
        std::uint8_t operation{};
        std::uint8_t status{};
        readUInt8(2, operation);
        readUInt8(3, status);
        if (operation != 1U) {
            return;
        }
        telemetryStatus_ = status;
        if (status == 1U && !messages_.isEmpty()) {
            const TelemetryMessage message = messages_.takeFirst();
            communicator_->sendMessageOnAllLinks(message.data);
        }
        return;
    }

    if (messageType_ != 0x89U || messageData_ == nullptr) {
        return;
    }
    if (messageSize_ > 13U) {
        lastLoraMessageId_ = readRawIntegral<quint64>(messageData_ + 6);
    }
    if (messageSize_ > 5U
        && static_cast<unsigned char>(messageData_[5]) == kRadarType) {
        emitTargetFromRetainedELinkMessage(false);
    }
}

void ELinkTelemetry::readTelemetryPendingDatagrams()
{
    if (telemetrySocket_ == nullptr) {
        return;
    }
    const QByteArray peek = QByteArray::fromHex("E00E77");
    const QByteArray lastWaypoint = QByteArray::fromHex("E00E78");
    const QByteArray setPosition = QByteArray::fromHex("E00E79");
    const QByteArray setLastWaypoint = QByteArray::fromHex("E00E80");
    const QByteArray search = QByteArray::fromHex("E00E81");
    const QByteArray destruct = QByteArray::fromHex("E00E82");

    while (telemetrySocket_->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<qsizetype>(
            telemetrySocket_->pendingDatagramSize()));
        telemetrySocket_->readDatagram(datagram.data(), datagram.size());

        if (datagram == peek) {
            emit peekAtGPS();
        } else if (datagram == lastWaypoint) {
            emit gotoLastWP();
        } else if (datagram.startsWith(setPosition)) {
            qlonglong time{};
            int lat{};
            int lon{};
            if (readIntegral(datagram, 3, time)
                && readIntegral(datagram, 11, lat)
                && readIntegral(datagram, 15, lon)) {
                emit setCurrentPos(time, lat, lon);
                emit resetVNav();
            }
        } else if (datagram.startsWith(setLastWaypoint)) {
            int lat{};
            int lon{};
            if (readIntegral(datagram, 3, lat)
                && readIntegral(datagram, 7, lon)) {
                emit setLastWPCoord(lat, lon);
            }
        } else if (datagram.startsWith(search)) {
            qlonglong time{};
            if (readIntegral(datagram, 3, time)) {
                emit startSearch(time);
            }
        } else if (datagram.startsWith(destruct)) {
            emit selfDestruct();
        }
    }
}

void ELinkTelemetry::readTelemetryPendingDatagrams2()
{
    if (telemetrySocket2_ == nullptr) {
        return;
    }
    const QByteArray sidAssignment = QByteArray::fromHex("E00E0001");
    const QByteArray radar = QByteArray::fromHex("E00E0013");
    const QByteArray remoteEnable = QByteArray::fromHex("E00E0006");
    const QByteArray remoteDisable = QByteArray::fromHex("E00E0007");
    const QByteArray manualLandStart = QByteArray::fromHex("E00E0008");
    const QByteArray manualLandStop = QByteArray::fromHex("E00E0009");
    const QByteArray interceptor = QByteArray::fromHex("E00E0010");

    while (telemetrySocket2_->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<qsizetype>(
            telemetrySocket2_->pendingDatagramSize()));
        telemetrySocket2_->readDatagram(datagram.data(), datagram.size());

        if (datagram.startsWith(sidAssignment)) {
            const QByteArray uid = planeUid_.toLocal8Bit();
            if (datagram.size() == uid.size() + 6
                && datagram.mid(4, uid.size()) == uid) {
                unsigned short sid{};
                if (readIntegral(datagram, 4 + uid.size(), sid)) {
                    planeSid_ = sid;
                }
            }
            continue;
        }

        if (datagram.startsWith(radar)) {
            // The original reads the ELinkAbstractHandler retained pointer,
            // not this UDP datagram (AArch64 load at RVA 0x38bd8).
            emitTargetFromRetainedELinkMessage(true);
            continue;
        }

        unsigned short sid{};
        if (planeSid_ == 0U || datagram.size() <= 4
            || !readIntegral(datagram, 4, sid) || sid != planeSid_) {
            continue;
        }

        if (datagram.startsWith(remoteEnable) && datagram.size() >= 13) {
            const unsigned char mode =
                static_cast<unsigned char>(datagram.at(6));
            std::int16_t roll{};
            std::int16_t pitch{};
            readIntegral(datagram, 7, roll);
            readIntegral(datagram, 9, pitch);
            const unsigned char throttleByte =
                static_cast<unsigned char>(datagram.at(11));
            const unsigned char options =
                static_cast<unsigned char>(datagram.at(12));
            const int throttle = mode == 0U
                ? static_cast<int>(throttleByte)
                : 1000 + 10 * static_cast<int>(throttleByte);
            emit remoteControlEnable(mode, roll, pitch, throttle, options);
        } else if (datagram.startsWith(remoteDisable)) {
            emit remoteControlDisable();
        } else if (datagram.startsWith(manualLandStart)) {
            emit startManuLand();
        } else if (datagram.startsWith(manualLandStop)) {
            emit stopManuLand();
        } else if (datagram.startsWith(interceptor)
                   && datagram.size() >= 7) {
            emit setInterceptorEnabled(datagram.at(6) != 0);
        }
    }
}

QString ELinkTelemetry::currentWifiName() const
{
    QProcess process;
    process.start(QStringLiteral("/bin/bash"),
                  {QStringLiteral("-c"),
                   QStringLiteral(
                       "nmcli con show | grep wifi | awk '{print $1}'")});
    process.waitForFinished(1000);
    return QString::fromUtf8(process.readAllStandardOutput().simplified());
}

void ELinkTelemetry::renameWifi(const QByteArray& datagram)
{
    QJsonParseError error{};
    const QJsonDocument document =
        QJsonDocument::fromJson(datagram.mid(5), &error);
    if (error.error != QJsonParseError::NoError) {
        return;
    }
    const QString newName =
        document.object().value(QStringLiteral("wifi")).toString();

    QString currentName = currentWifiName();
    while (!currentName.isEmpty()) {
        QProcess deleteProcess;
        deleteProcess.start(
            QStringLiteral("/bin/bash"),
            {QStringLiteral("-c"),
             QStringLiteral("nmcli con delete %1").arg(currentName)});
        deleteProcess.waitForFinished(1000);
        currentName = currentWifiName();
    }

    QProcess createProcess;
    createProcess.start(
        QStringLiteral("/bin/bash"),
        {QStringLiteral("-c"),
         QStringLiteral("/home/jetson/mkwlan.sh %1").arg(newName)});
    createProcess.waitForFinished(10000);
}

void ELinkTelemetry::readEPLaunchDatagrams()
{
    if (epLaunchSocket_ == nullptr) {
        return;
    }
    const QByteArray launcherDiscovery = QByteArray::fromHex("C00C010203");
    const QByteArray configuratorDiscovery = QByteArray::fromHex("C00C010204");
    const QByteArray rename = QByteArray::fromHex("C00C010205");
    const QByteArray reboot = QByteArray::fromHex("C00C01020600");

    while (epLaunchSocket_->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<qsizetype>(
            epLaunchSocket_->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort{};
        epLaunchSocket_->readDatagram(datagram.data(),
                                      datagram.size(),
                                      &sender,
                                      &senderPort);
        Q_UNUSED(senderPort)
        const QString senderAddress = sender.toString();

        if (datagram == launcherDiscovery) {
            if (epLauncherAddress_ != senderAddress) {
                epLauncherAddress_ = senderAddress;
            }
        } else if (datagram == configuratorDiscovery) {
            if (epConfiguratorAddress_ != senderAddress) {
                epConfiguratorAddress_ = senderAddress;
            }
        } else if (datagram.startsWith(rename)) {
            renameWifi(datagram);
        } else if (datagram == reboot) {
            QProcess::startDetached(QStringLiteral("reboot"), {});
        }
    }
}

void ELinkTelemetry::pingTelemetry()
{
    if (pingProcess_ == nullptr) {
        return;
    }
    pingProcess_->start(QStringLiteral("ping"),
                        {hostAddress_, QStringLiteral("-c"),
                         QStringLiteral("1")});
}

void ELinkTelemetry::pingTelemetryFinished(
    const int exitCode,
    QProcess::ExitStatus)
{
    if (exitCode == 0) {
        if (!connected_) {
            emit telemetryConnected();
        }
        connected_ = true;
    } else {
        if (connected_) {
            emit telemetryDisconnected();
        }
        connected_ = false;
        connectToUDPTelemetry();
    }
    QTimer::singleShot(connected_ ? 10000 : 2000,
                       this,
                       &ELinkTelemetry::pingTelemetry);
}

void ELinkTelemetry::timerEvent(QTimerEvent* event)
{
    Q_UNUSED(event)
    if (!directUdpMode_) {
        getTelemetryStatus();
    } else if (telemetrySocket_ != nullptr && !messages_.isEmpty()) {
        const TelemetryMessage message = messages_.takeFirst();
        telemetrySocket_->writeDatagram(
            message.data, QHostAddress(hostAddress_), telemetryPort_);
    }

    const qlonglong now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastLoraPingTime_ > 2000) {
        lastLoraPingTime_ = now;
        QByteArray body;
        appendIntegral(body, lastLoraMessageId_);
        sendProtocolMessage(0x8aU, body);
    }

    if (now - launcherEpoch_ > launcherIntervalMs_ && !launched_
        && epLaunchSocket_ != nullptr && !epLauncherAddress_.isEmpty()) {
        QByteArray data;
        data.append(static_cast<char>(0xb0));
        data.append(static_cast<char>(0x0b));
        data.append(static_cast<char>(currentMode_));
        unsigned char flags = launchReady_ ? 1U : 0U;
        if (planPresent_) {
            flags |= 2U;
        }
        data.append(static_cast<char>(flags));
        epLaunchSocket_->writeDatagram(
            data, QHostAddress(epLauncherAddress_), epLaunchSendPort_);
    }

    if (now - configuratorEpoch_ > configuratorIntervalMs_ && !launched_
        && epLaunchSocket_ != nullptr
        && !epConfiguratorAddress_.isEmpty()) {
        QJsonObject object;
        object.insert(QStringLiteral("wifi"), currentWifiName());
        QByteArray data;
        data.append(static_cast<char>(0xb0));
        data.append(static_cast<char>(0x0c));
        data.append(QJsonDocument(object).toJson());
        epLaunchSocket_->writeDatagram(
            data, QHostAddress(epConfiguratorAddress_), epLaunchSendPort_);
    }
}
