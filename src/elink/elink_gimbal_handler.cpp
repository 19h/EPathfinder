#include "elink/elink_gimbal_handler.hpp"

#include <QByteArray>
#include <QTimer>

ELinkGimbalHandler::ELinkGimbalHandler(
    ELinkCommunicator* const communicator)
    : ELinkAbstractHandler(communicator)
{
}

void ELinkGimbalHandler::processMessage()
{
    if (messageType_ != 0x87U) {
        return;
    }

    std::uint8_t axis0Mode = 0U;
    std::int16_t magneticAngle0 = 0;
    std::int16_t imuAngle0 = 0;
    std::uint8_t axis1Mode = 0U;
    std::int16_t magneticAngle1 = 0;
    std::int16_t imuAngle1 = 0;
    readUInt8(2, axis0Mode);
    readInt16(3, magneticAngle0);
    readInt16(5, imuAngle0);
    readUInt8(7, axis1Mode);
    readInt16(8, magneticAngle1);
    readInt16(10, imuAngle1);
    Q_UNUSED(axis0Mode)
    Q_UNUSED(magneticAngle0)
    Q_UNUSED(imuAngle0)
    Q_UNUSED(axis1Mode)
    Q_UNUSED(magneticAngle1)
    Q_UNUSED(imuAngle1)
}

void ELinkGimbalHandler::setRollFixed(const bool fixed)
{
    QByteArray body(4, '\0');
    body[0] = static_cast<char>(rollAxis_);
    body[3] = static_cast<char>(fixed);
    sendProtocolMessage(0x88U, body);
}

void ELinkGimbalHandler::fixRoll()
{
    setRollFixed(true);
}

void ELinkGimbalHandler::unfixRoll()
{
    setRollFixed(false);
}

void ELinkGimbalHandler::setPitch(const float angle, const bool fixed)
{
    std::int16_t encodedAngle = 0;
    if (!fixed) {
        encodedAngle = static_cast<std::int16_t>(
            static_cast<int>(angle * 100.0F));
    }

    QByteArray body(4, '\0');
    body[0] = static_cast<char>(pitchAxis_);
    const auto raw = static_cast<std::uint16_t>(encodedAngle);
    body[1] = static_cast<char>(raw & 0xffU);
    body[2] = static_cast<char>((raw >> 8U) & 0xffU);
    body[3] = static_cast<char>(fixed);
    sendProtocolMessage(0x88U, body);
}

void ELinkGimbalHandler::setServoPTZ(const float roll,
                                     const float pitch,
                                     const float yaw,
                                     const float rollMaxSpeed,
                                     const float pitchMaxSpeed,
                                     const float yawMaxSpeed,
                                     const bool center,
                                     const bool bYawByNorth,
                                     const int gimbalId)
{
    Q_UNUSED(roll)
    Q_UNUSED(yaw)
    Q_UNUSED(rollMaxSpeed)
    Q_UNUSED(pitchMaxSpeed)
    Q_UNUSED(yawMaxSpeed)
    Q_UNUSED(bYawByNorth)
    Q_UNUSED(gimbalId)
    setPitch(pitch, center);
    if (center) {
        QTimer::singleShot(50, this, &ELinkGimbalHandler::fixRoll);
    } else {
        QTimer::singleShot(50, this, &ELinkGimbalHandler::unfixRoll);
    }
}
