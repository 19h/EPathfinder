#include "elink/elink_communicator.hpp"

#include "link/abstract_link.hpp"

#include <algorithm>

ELinkCommunicator::ELinkCommunicator(QObject* const parent)
    : QObject(parent)
{
}

std::uint16_t ELinkCommunicator::messageDelay() const
{
    return messageDelayMs_;
}

QList<AbstractLink*> ELinkCommunicator::links() const
{
    return links_;
}

void ELinkCommunicator::sendMessage(const QByteArray& message,
                                    AbstractLink* const link)
{
    link->sendData(message);
}

void ELinkCommunicator::sendMessageOnAllLinks(const QByteArray& message)
{
    for (AbstractLink* const link : links_) {
        sendMessage(message, link);
    }
}

void ELinkCommunicator::addMessageTimeDelay(const int value)
{
    messageDelayMs_ = static_cast<std::uint16_t>(messageDelayMs_ + value);
}

void ELinkCommunicator::removeLink(AbstractLink* const link)
{
    if (!links_.contains(link)) {
        return;
    }
    links_.removeAll(link);
    disconnect(link,
               &AbstractLink::dataReceived,
               this,
               &ELinkCommunicator::onDataReceived);
}

void ELinkCommunicator::addLink(AbstractLink* const link)
{
    if (links_.contains(link)) {
        return;
    }
    links_.append(link);
    connect(link,
            &AbstractLink::dataReceived,
            this,
            &ELinkCommunicator::onDataReceived);
}

void ELinkCommunicator::onDataReceived(const QByteArray& data)
{
    for (const char rawByte : data) {
        const std::uint8_t byte = static_cast<std::uint8_t>(rawByte);
        switch (framePosition_) {
        case 0:
            if (byte == 0xd0U) {
                framePosition_ = 1;
            }
            break;
        case 1:
            if (byte == 0x0dU) {
                framePosition_ = 2;
            } else {
                framePosition_ = 0;
            }
            break;
        case 2:
            messageLength_ = byte;
            framePosition_ = 3;
            break;
        case 3:
            messageLength_ = static_cast<std::uint16_t>(
                messageLength_ | (static_cast<std::uint16_t>(byte) << 8U));
            if (messageLength_ == 0U) {
                framePosition_ = 0;
                break;
            }
            currentMessage_.clear();
            currentMessage_.reserve(messageLength_);
            framePosition_ = 4;
            break;
        default:
            if (framePosition_ == 4U) {
                messageByte0_ = byte;
            } else if (framePosition_ == 5U) {
                messageByte1_ = byte;
            }
            ++framePosition_;
            currentMessage_.append(rawByte);
            if (framePosition_
                == static_cast<std::uint16_t>(messageLength_ + 4U)) {
                framePosition_ = 0;
                emit messageReceived(currentMessage_);
            }
            break;
        }
    }
}
