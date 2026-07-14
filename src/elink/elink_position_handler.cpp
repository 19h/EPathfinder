#include "elink/elink_position_handler.hpp"

#include <QByteArray>

#include <cstring>

namespace {

void appendFloatLittleEndian(QByteArray& output, const float value)
{
    std::uint32_t raw = 0U;
    static_assert(sizeof(raw) == sizeof(value));
    std::memcpy(&raw, &value, sizeof(raw));
    output.append(static_cast<char>(raw & 0xffU));
    output.append(static_cast<char>((raw >> 8U) & 0xffU));
    output.append(static_cast<char>((raw >> 16U) & 0xffU));
    output.append(static_cast<char>((raw >> 24U) & 0xffU));
}

} // namespace

ELinkPositionHandler::ELinkPositionHandler(
    ELinkCommunicator* const communicator)
    : ELinkAbstractHandler(communicator)
{
}

void ELinkPositionHandler::processMessage()
{
    switch (messageType_) {
    case 0x00U: {
        int pressure = 0;
        int temperature = 0;
        readInt(2, pressure);
        readInt(6, temperature);
        emit barometricRawEvent(messageTime_, pressure, temperature);
        break;
    }
    case 0x30U: {
        int x = 0;
        int y = 0;
        int z = 0;
        readInt(2, x);
        readInt(6, y);
        readInt(10, z);
        emit magneticEvent(messageTime_, x, y, z);
        break;
    }
    case 0x40U:
    case 0x41U: {
        std::uint8_t status = 0U;
        std::uint8_t satellites = 0U;
        readUInt8(2, status);
        readUInt8(3, satellites);

        int altitude = 0;
        int latitude = 0;
        int longitude = 0;
        float velocityX = 0.0F;
        float velocityY = 0.0F;
        float velocityZ = 0.0F;
        float groundSpeed = 0.0F;
        float groundCourse = 0.0F;
        readInt(4, altitude);
        readInt(8, latitude);
        readInt(12, longitude);
        readFloat(16, velocityX);
        readFloat(20, velocityY);
        readFloat(24, velocityZ);
        readFloat(28, groundSpeed);
        readFloat(32, groundCourse);

        std::uint32_t systemCounter = 0U;
        std::uint32_t positionCounter = 0U;
        if (messageType_ == 0x41U || messageSize_ == 0x2eU) {
            readUInt32(36, systemCounter);
            readUInt32(40, positionCounter);
        }
        Q_UNUSED(systemCounter)
        Q_UNUSED(positionCounter)

        if (messageType_ == 0x40U) {
            emit positionEvent(messageTime_,
                               status,
                               satellites,
                               latitude,
                               longitude,
                               altitude,
                               velocityX,
                               velocityY,
                               velocityZ,
                               groundSpeed,
                               groundCourse);
        } else {
            emit backupPositionEvent(messageTime_,
                                     status,
                                     satellites,
                                     latitude,
                                     longitude,
                                     altitude,
                                     velocityX,
                                     velocityY,
                                     velocityZ,
                                     groundSpeed,
                                     groundCourse);
        }
        break;
    }
    case 0x50U: {
        int altitude = 0;
        readInt(2, altitude);
        emit distanceSensorEvent(100U,
                                 300000U,
                                 static_cast<unsigned int>(altitude / 10));
        break;
    }
    case 0x51U: {
        int altitude = 0;
        readInt(2, altitude);
        emit barometricAltEvent(messageTime_, altitude);
        break;
    }
    case 0x5fU: {
        std::uint8_t status = 0U;
        readUInt8(2, status);
        if (!accelerationDetected_ && status != 0U) {
            accelerationDetected_ = true;
            emit launchAcceleration();
        }
        break;
    }
    case 0x77U: {
        // The original decodes this IMU report solely for debug output.
        int values[4]{};
        float floatValues[4]{};
        std::uint8_t status = 0U;
        readInt(2, values[0]);
        readInt(6, values[1]);
        readInt(10, values[2]);
        readInt(14, values[3]);
        readFloat(18, floatValues[0]);
        readFloat(22, floatValues[1]);
        readFloat(26, floatValues[2]);
        readFloat(30, floatValues[3]);
        readUInt8(34, status);
        Q_UNUSED(values)
        Q_UNUSED(floatValues)
        Q_UNUSED(status)
        break;
    }
    default:
        break;
    }
}

void ELinkPositionHandler::sendLaunchEvent()
{
    sendProtocolMessage(0x03U, QByteArray(1, '\0'));
}

void ELinkPositionHandler::setManualAccelerationEvent()
{
    accelerationDetected_ = true;
    sendLaunchEvent();
}

void ELinkPositionHandler::sendAirspeed(const float value)
{
    QByteArray body;
    body.reserve(4);
    appendFloatLittleEndian(body, value);
    sendProtocolMessage(0x22U, body);
}
