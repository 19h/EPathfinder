#include "elink/elink_movement_handler.hpp"

ELinkMovemenetHandler::ELinkMovemenetHandler(
    ELinkCommunicator* const communicator)
    : ELinkAbstractHandler(communicator)
{
}

void ELinkMovemenetHandler::processMessage()
{
    switch (messageType_) {
    case 0x10U: {
        int xacc = 0;
        int yacc = 0;
        int zacc = 0;
        int temperature = 0;
        readInt(2, xacc);
        readInt(6, yacc);
        readInt(10, zacc);
        // This duplicated offset is present in the source binary at
        // 0x15d6b4..0x15d6c0; preserve it rather than inferring offset 14.
        readInt(10, temperature);
        emit accelerationEvent(messageTime_,
                               static_cast<unsigned int>(xacc),
                               static_cast<unsigned int>(yacc),
                               static_cast<unsigned int>(zacc),
                               temperature);
        break;
    }
    case 0x20U: {
        int pressure = 0;
        int temperature = 0;
        readInt(2, pressure);
        readInt(6, temperature);
        emit airspeedRawEvent(messageTime_, pressure, temperature);
        break;
    }
    case 0x21U: {
        float speed = 0.0F;
        readFloat(2, speed);
        emit airspeedEvent(messageTime_, speed);
        break;
    }
    default:
        break;
    }
}
