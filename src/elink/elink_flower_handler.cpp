#include "elink/elink_flower_handler.hpp"

#include <QByteArray>
#include <QDateTime>
#include <QTimerEvent>

namespace {

void appendInt32LittleEndian(QByteArray& output, const std::int32_t value)
{
    const auto raw = static_cast<std::uint32_t>(value);
    output.append(static_cast<char>(raw & 0xffU));
    output.append(static_cast<char>((raw >> 8U) & 0xffU));
    output.append(static_cast<char>((raw >> 16U) & 0xffU));
    output.append(static_cast<char>((raw >> 24U) & 0xffU));
}

} // namespace

ELinkFlowerHandler::ELinkFlowerHandler(
    ELinkCommunicator* const communicator)
    : ELinkAbstractHandler(communicator)
{
}

bool ELinkFlowerHandler::isConnected() const
{
    return QDateTime::currentMSecsSinceEpoch() - lastMessageTime_ <= 4999;
}

void ELinkFlowerHandler::processMessage()
{
    if (messageType_ != 0x68U) {
        return;
    }

    const qlonglong now = QDateTime::currentMSecsSinceEpoch();
    std::int16_t rollRaw = 0;
    std::int16_t pitchRaw = 0;
    std::uint16_t yawRaw = 0U;
    std::uint8_t gnss = 0U;
    int latitude = 0;
    int longitude = 0;
    std::uint16_t headingRaw = 0U;
    std::uint16_t groundSpeedRaw = 0U;
    std::uint16_t airspeedRaw = 0U;
    int altitudeRaw = 0;
    readInt16(2, rollRaw);
    readInt16(4, pitchRaw);
    readUInt16(6, yawRaw);
    readUInt8(8, gnss);
    readInt(9, latitude);
    readInt(13, longitude);
    readUInt16(17, headingRaw);
    readUInt16(19, groundSpeedRaw);
    readUInt16(21, airspeedRaw);
    readInt(23, altitudeRaw);

    const float roll = static_cast<float>(rollRaw) / 100.0F;
    const float pitch = static_cast<float>(pitchRaw) / 100.0F;
    const float yaw = static_cast<float>(yawRaw) / 100.0F;
    const float heading = static_cast<float>(headingRaw) / 100.0F;
    const float groundSpeed = static_cast<float>(groundSpeedRaw) / 36.0F;
    const float airspeed = static_cast<float>(airspeedRaw) / 36.0F;
    const int altitude = 100 * altitudeRaw;

    lastMessageTime_ = now;
    gnssStatus_ = gnss;
    emit currentAHRS(roll,
                     pitch,
                     yaw,
                     gnss,
                     latitude,
                     longitude,
                     altitude,
                     heading,
                     groundSpeed,
                     airspeed);
    if (gnss == 1U && now - lastGpsEventTime_ > 1000) {
        emit currentGPS(latitude, longitude);
        lastGpsEventTime_ = now;
    }
}

void ELinkFlowerHandler::sendCurrentCoordinate(const qlonglong time,
                                                const int latitude,
                                                const int longitude,
                                                const float confidence)
{
    Q_UNUSED(time)
    Q_UNUSED(confidence)
    const qlonglong now = QDateTime::currentMSecsSinceEpoch();
    if (gnssStatus_ == 1U && now - lastMessageTime_ <= 999) {
        return;
    }

    QByteArray body;
    body.reserve(8);
    appendInt32LittleEndian(body, latitude);
    appendInt32LittleEndian(body, longitude);
    sendProtocolMessage(0x48U, body);
}

void ELinkFlowerHandler::timerEvent(QTimerEvent* const event)
{
    Q_UNUSED(event)
}
