#include "elink/elink_interceptor_handler.hpp"

ELinkInterceptorHandler::ELinkInterceptorHandler(
    ELinkCommunicator* const communicator)
    : ELinkAbstractHandler(communicator)
{
}

void ELinkInterceptorHandler::processMessage()
{
    if (messageType_ != 0x9aU) {
        return;
    }

    bool targetPresent = false;
    const int detectionByteCount = static_cast<int>(messageSize_) - 2;
    for (int i = 0; i < detectionByteCount; ++i) {
        std::uint8_t value = 0U;
        readUInt8(i + 2, value);
        if (value != 0U) {
            targetPresent = true;
        }
    }

    if (targetPresent_ != targetPresent) {
        targetPresent_ = targetPresent;
        if (targetPresent_) {
            emit targetDetected();
        }
    }
}
