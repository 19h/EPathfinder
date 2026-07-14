#include "elink/elink_attitude_handler.hpp"

#include <QByteArray>

ELinkAttitudeHandler::ELinkAttitudeHandler(
    ELinkCommunicator* const communicator)
    : ELinkAbstractHandler(communicator)
{
}

void ELinkAttitudeHandler::processMessage()
{
    switch (messageType_) {
    case 0x60U: {
        float pitch = 0.0F;
        float roll = 0.0F;
        float yaw = 0.0F;
        readFloat(2, pitch);
        readFloat(6, roll);
        readFloat(10, yaw);
        constexpr float radiansToDegrees = 57.296F;
        pitch *= radiansToDegrees;
        roll *= radiansToDegrees;
        yaw *= radiansToDegrees;
        if (yaw < 0.0F) {
            yaw += 360.0F;
        } else if (yaw >= 360.0F) {
            yaw -= 360.0F;
        }
        // RVA 0x15a088 only logs these decoded values. Despite being in the
        // meta-object, attitudeEvent is not emitted on this path.
        break;
    }
    case 0x65U: {
        std::uint8_t result = 0U;
        readUInt8(2, result);
        emit mgReseted(result);
        break;
    }
    case 0x0fU: {
        std::uint32_t systemCounter = 0U;
        std::uint8_t status = 0U;
        std::uint32_t mgCounter = 0U;
        readUInt32(2, systemCounter);
        readUInt8(6, status);
        readUInt32(7, mgCounter);
        Q_UNUSED(systemCounter)
        Q_UNUSED(mgCounter)
        emit mgCfgStatus(status);
        if (lastMgStatus_ != status) {
            lastMgStatus_ = status;
            if (lastMgStatus_ == 0x0bU) {
                emit mgInited();
            }
        }
        break;
    }
    default:
        break;
    }
}

void ELinkAttitudeHandler::mgReset()
{
    sendProtocolMessage(0x65U, QByteArray(1, '\0'));
}
