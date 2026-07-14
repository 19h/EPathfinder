#include "elink/elink_arm_handler.hpp"

#include <QByteArray>
#include <QTimer>

ELinkArmHandler::ELinkArmHandler(ELinkCommunicator* const communicator)
    : ELinkAbstractHandler(communicator)
{
}

void ELinkArmHandler::sendMsg(const std::uint8_t id,
                              const std::uint8_t value)
{
    sendProtocolMessage(id, QByteArray(1, static_cast<char>(value)));
}

void ELinkArmHandler::processMessage()
{
    if (messageType_ != 0x90U) {
        return;
    }

    std::uint8_t state = 0U;
    readUInt8(2, state);
    if (armTestState_ != state) {
        armTestState_ = state;
    }
    if (state != 0U) {
        sendMsg(0x90U, 0x5aU);
    }
}

void ELinkArmHandler::sendArmMsg()
{
    sendMsg(0x91U, 0x6bU);
    if (selfDestruction_) {
        sendMsg(0x92U, 0x7cU);
    }

    ++sendCounter_;
    if (sendCounter_ <= 19 || selfDestruction_) {
        QTimer::singleShot(100, this, &ELinkArmHandler::sendArmMsg);
    } else {
        armed_ = false;
    }
}

void ELinkArmHandler::armOn()
{
    if (armed_) {
        return;
    }
    sendCounter_ = 0;
    armed_ = true;
    sendArmMsg();
}

void ELinkArmHandler::selfDestruction()
{
    if (selfDestruction_) {
        return;
    }
    selfDestruction_ = true;
    armOn();
}
