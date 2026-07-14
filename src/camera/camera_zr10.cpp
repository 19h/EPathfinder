#include "camera/camera_zr10.hpp"

#include "link/serial_link.hpp"

#include <QDateTime>
#include <QHostAddress>
#include <QNetworkDatagram>
#include <QTimer>
#include <QTimerEvent>
#include <QUdpSocket>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {

void appendLe16(QByteArray& data, std::uint16_t value)
{
    data.append(static_cast<char>(value & 0xffU));
    data.append(static_cast<char>((value >> 8U) & 0xffU));
}

std::uint16_t readLe16(const char* data)
{
    return static_cast<std::uint16_t>(
        static_cast<unsigned char>(data[0]) |
        (static_cast<unsigned int>(static_cast<unsigned char>(data[1])) << 8U));
}

std::int16_t readLeI16(const char* data)
{
    return static_cast<std::int16_t>(readLe16(data));
}

} // namespace

CameraZR10::CameraZR10(const QString& address,
                       int localPort,
                       int remotePort,
                       bool externalGimbalControl,
                       QObject* parent)
    : Camera(parent)
    , udpSocket_(new QUdpSocket(this))
    , address_(address)
    , localPort_(localPort)
    , remotePort_(remotePort)
    , pingProcess_(new QProcess(this))
    , externalGimbalControl_(externalGimbalControl)
{
    setControlFeatures(true, externalGimbalControl);
    appendCalibrations();

    QObject::connect(udpSocket_, &QUdpSocket::readyRead,
                     this, &CameraZR10::readPendingDatagrams);
    QObject::connect(
        pingProcess_,
        qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
        this,
        &CameraZR10::pingFinished);
    startTimer(static_cast<int>(gimbalReadIntervalMs_), Qt::CoarseTimer);
    QTimer::singleShot(500, this, &CameraZR10::ping);
}

void CameraZR10::appendCalibrations()
{
    struct Calibration {
        float optical;
        float percent;
        float width;
        float height;
    };
    // Exact float32 records emitted by the constructor at RVA 0x2c678.
    static constexpr Calibration values[] = {
        {1.5F, 0.0F, 44.4069595F, 25.1380806F},
        {2.4000001F, 3.15789509F, 28.4573402F, 16.2028999F},
        {2.5999999F, 3.85964894F, 26.8034306F, 15.1126804F},
        {3.20000005F, 5.96491194F, 20.4192696F, 11.7683897F},
        {3.4000001F, 6.66666698F, 20.1926708F, 11.6526804F},
        {3.5999999F, 7.36842108F, 18.4312897F, 10.6681805F},
        {4.30000019F, 9.82456112F, 15.8015699F, 8.86894035F},
        {4.5999999F, 10.8771935F, 14.8828096F, 8.3457098F},
        {5.19999981F, 12.9824562F, 12.8086996F, 7.2400198F},
        {6.0999999F, 16.1403503F, 11.3053999F, 6.42440987F},
        {6.80000019F, 18.5964909F, 10.1463203F, 5.66648006F},
        {7.9000001F, 22.4561405F, 8.69456959F, 4.96641016F},
        {9.0F, 26.3157883F, 7.70574999F, 4.38274002F},
        {10.0F, 29.8245602F, 7.12353992F, 3.9156301F},
        {12.0F, 36.8421059F, 6.71577978F, 3.79883003F},
        {13.0F, 40.3508759F, 6.6575098F, 3.74043012F},
        {16.0F, 50.8771935F, 6.19125986F, 3.50679994F},
        {18.0F, 57.8947372F, 5.78312016F, 3.33156991F},
        {20.0F, 64.9122772F, 5.4914999F, 3.03946996F},
        {22.8999996F, 75.0877228F, 5.02476978F, 2.80575991F},
        {24.0F, 78.9473648F, 4.79133987F, 2.68889999F},
        {26.0F, 85.9649124F, 4.38274002F, 2.45514989F},
        {28.0F, 92.9824524F, 4.14919996F, 2.33826995F},
        {30.0F, 100.0F, 3.68202996F, 2.10450006F},
    };
    for (const Calibration& item : values) {
        Zoom zoom{item.optical, item.percent, item.width, item.height,
                  1.0F, 1.0F, 0};
        zoom.calibration.scaleX = 1.0F;
        zoom.calibration.scaleY = 1.0F;
        calibrationZooms().append(zoom);
    }
}

unsigned short CameraZR10::CRC16_cal(unsigned char* data,
                                     unsigned int length,
                                     unsigned short initialValue)
{
    // This is the table update used at RVA 0x29f48. The copied table begins
    // 0000,1021,2042,3063, identifying polynomial 0x1021.
    std::uint16_t crc = initialValue;
    for (unsigned int i = 0; i < length; ++i) {
        crc ^= static_cast<std::uint16_t>(data[i]) << 8U;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x8000U) != 0U
                ? static_cast<std::uint16_t>((crc << 1U) ^ 0x1021U)
                : static_cast<std::uint16_t>(crc << 1U);
        }
    }
    return crc;
}

QByteArray CameraZR10::makePacket(unsigned char command,
                                  const QByteArray& payload)
{
    QByteArray packet;
    packet.reserve(payload.size() + 10);
    packet.append(static_cast<char>(kStartLow));
    packet.append(static_cast<char>(kStartHigh));
    packet.append(static_cast<char>(kVersion));
    appendLe16(packet, static_cast<std::uint16_t>(payload.size()));
    appendLe16(packet, sequence_++);
    packet.append(static_cast<char>(command));
    packet.append(payload);
    const auto crc = CRC16_cal(
        reinterpret_cast<unsigned char*>(packet.data()),
        static_cast<unsigned int>(packet.size()),
        0);
    appendLe16(packet, crc);
    return packet;
}

void CameraZR10::sendPacket(const QByteArray& packet)
{
    if (serialLink_ && serialTransport_) {
        serialLink_->sendData(packet);
        return;
    }
    udpSocket_->writeDatagram(packet, QHostAddress(address_),
                              static_cast<quint16>(remotePort_));
}

void CameraZR10::setSerialLink(const QString& portName, int baudRate)
{
    if (serialLink_) {
        serialLink_->deleteLater();
    }
    serialLink_ = new SerialLink(portName, baudRate, this);
    QObject::connect(serialLink_, &SerialLink::dataReceived,
                     this, &CameraZR10::serialLinkDataReceived);
    serialTransport_ = true;
    serialLink_->up();
}

void CameraZR10::connect()
{
    if (udpSocket_->state() == QAbstractSocket::UnconnectedState) {
        udpSocket_->bind(QHostAddress(address_),
                         static_cast<quint16>(localPort_),
                         QUdpSocket::ShareAddress |
                             QUdpSocket::ReuseAddressHint);
    }
    setAutoFocus();
    setZoom(0.0F);
    setConnectedState(true);
}

void CameraZR10::disconnect()
{
    if (!isConnected()) {
        return;
    }
    udpSocket_->close();
    setConnectedState(false);
}

void CameraZR10::setFocus(float distance,
                          float zoomPercent,
                          int timeoutMSec)
{
    Q_UNUSED(distance)
    Q_UNUSED(zoomPercent)
    Q_UNUSED(timeoutMSec)
}

void CameraZR10::setZoom(float percent)
{
    if (calibrationZooms().isEmpty()) {
        return;
    }
    const Zoom* selected = nullptr;
    for (const Zoom& item : calibrationZooms()) {
        if (item.percent >= percent) {
            selected = &item;
            break;
        }
    }
    if (!selected) {
        return;
    }
    if (zoomCommandInitialized_ && zoom().percent == percent) {
        return;
    }

    const int optical = std::clamp(static_cast<int>(selected->opticalZoom),
                                   1, 30);
    QByteArray payload;
    payload.append(static_cast<char>(optical));
    payload.append(char{0});
    sendPacket(makePacket(kZoomCommand, payload));
    applyZoomRecord(*selected,
                    QDateTime::currentMSecsSinceEpoch() -
                        zoomTimestampCorrectionMs_);
    zoomCommandInitialized_ = true;
}

void CameraZR10::setAutoCentering(bool enabled)
{
    QByteArray payload;
    payload.append(static_cast<char>(enabled));
    sendPacket(makePacket(kAutoCenterCommand, payload));
    autoCentering_ = enabled;
}

void CameraZR10::setGimbal(const Gimbal& value)
{
    if (externalGimbalControl_) {
        Camera::setGimbal(value);
        return;
    }
    if (autoCentering_ != value.autoCenter) {
        setAutoCentering(value.autoCenter);
    }
    const auto yaw = static_cast<std::int16_t>(-value.yaw * 10.0F);
    const auto pitch = static_cast<std::int16_t>(value.pitch * 10.0F);
    QByteArray payload;
    appendLe16(payload, static_cast<std::uint16_t>(yaw));
    appendLe16(payload, static_cast<std::uint16_t>(pitch));
    sendPacket(makePacket(kSetGimbalCommand, payload));
}

void CameraZR10::setAutoFocus()
{
    sendPacket(makePacket(kAutoFocusCommand, QByteArray(1, char{1})));
    QTimer::singleShot(5000, this, &CameraZR10::setAutoFocus);
}

void CameraZR10::readGimbal()
{
    sendPacket(makePacket(kReadGimbalCommand, {}));
}

void CameraZR10::timerEvent(QTimerEvent* event)
{
    Q_UNUSED(event)
    if (isConnected() && !externalGimbalControl_ &&
        QDateTime::currentMSecsSinceEpoch() - lastGimbalMessageMs_ >
            gimbalReadIntervalMs_) {
        readGimbal();
    }
}

void CameraZR10::ping()
{
    if (pingProcess_->state() != QProcess::NotRunning) {
        return;
    }
    pingProcess_->start(QStringLiteral("ping"),
                        {address_, QStringLiteral("-c"), QStringLiteral("1")});
}

void CameraZR10::pingFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    setDeviceEnabledState(exitCode == 0 &&
                          exitStatus == QProcess::NormalExit);
    QTimer::singleShot(1000, this, &CameraZR10::ping);
}

void CameraZR10::readPendingDatagrams()
{
    const qlonglong time = QDateTime::currentMSecsSinceEpoch() -
        zoomTimestampCorrectionMs_;
    while (udpSocket_->hasPendingDatagrams()) {
        receiveData_ = udpSocket_->receiveDatagram().data();
        parseData(time, receiveData_.size());
    }
}

void CameraZR10::parseData(qlonglong time, int size)
{
    if (size <= 0 || receiveData_.isEmpty()) {
        return;
    }
    int offset = 0;
    const int limit = std::min(size, static_cast<int>(receiveData_.size()));
    while (offset + 10 <= limit) {
        const char* cursor = receiveData_.constData() + offset;
        if (static_cast<unsigned char>(cursor[0]) != kStartLow ||
            static_cast<unsigned char>(cursor[1]) != kStartHigh) {
            ++offset;
            continue;
        }
        const int frameSize = 10 + readLe16(cursor + 3);
        if (frameSize < 10 || offset + frameSize > limit) {
            break;
        }
        parseFrame(receiveData_.mid(offset, frameSize), time);
        offset += frameSize;
    }
}

void CameraZR10::parseFrame(const QByteArray& frame, qlonglong time)
{
    if (frame.size() < 10 ||
        static_cast<unsigned char>(frame[0]) != kStartLow ||
        static_cast<unsigned char>(frame[1]) != kStartHigh) {
        return;
    }
    const int payloadSize = readLe16(frame.constData() + 3);
    if (frame.size() != payloadSize + 10) {
        return;
    }
    const auto expected = readLe16(frame.constData() + frame.size() - 2);
    const auto actual = CRC16_cal(
        reinterpret_cast<unsigned char*>(const_cast<char*>(frame.constData())),
        static_cast<unsigned int>(frame.size() - 2), 0);
    if (actual != expected) {
        return;
    }

    const auto command = static_cast<unsigned char>(frame[7]);
    const char* payload = frame.constData() + 8;
    if (command == kZoomCommand && payloadSize >= 2) {
        updateZoomFromOptical(
            static_cast<float>(readLe16(payload)) / 10.0F, time);
        return;
    }
    if ((command != kReadGimbalCommand && command != kSetGimbalCommand) ||
        payloadSize < 6 || externalGimbalControl_) {
        return;
    }

    const float yaw = static_cast<float>(readLeI16(payload)) / -10.0F;
    const float pitch = static_cast<float>(readLeI16(payload + 2)) / 10.0F;
    const float roll = static_cast<float>(readLeI16(payload + 4)) / 10.0F;
    const Gimbal previous = gimbal();
    const qlonglong deltaMs = time - previous.time;
    if (deltaMs <= 0) {
        return;
    }
    const float rate = 1000.0F / static_cast<float>(deltaMs);
    updateGimbalTelemetry(roll, pitch, yaw,
                          (roll - previous.roll) * rate,
                          (pitch - previous.pitch) * rate,
                          (yaw - previous.yaw) * rate,
                          time);
    lastGimbalMessageMs_ = time;
}

void CameraZR10::serialLinkDataReceived(const QByteArray& data)
{
    serialBuffer_.append(data);
    while (serialBuffer_.size() >= 10) {
        const int start = serialBuffer_.indexOf(QByteArray::fromHex("5566"));
        if (start < 0) {
            serialBuffer_.clear();
            return;
        }
        if (start > 0) {
            serialBuffer_.remove(0, start);
        }
        if (serialBuffer_.size() < 10) {
            return;
        }
        const int frameSize = 10 + readLe16(serialBuffer_.constData() + 3);
        if (serialBuffer_.size() < frameSize) {
            return;
        }
        receiveData_ = serialBuffer_.left(frameSize);
        parseData(QDateTime::currentMSecsSinceEpoch(), frameSize);
        serialBuffer_.remove(0, frameSize);
    }
}
