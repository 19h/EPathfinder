#include "mavlink/mavlink_communicator.hpp"

#include <QDateTime>
#include <QDebug>

#include <array>

MavLinkCommunicator::MavLinkCommunicator(const uint8_t systemId,
                                         const uint8_t componentId,
                                         QObject* const parent)
    : QObject(parent)
    , systemId_(systemId)
    , componentId_(componentId)
{
    qRegisterMetaType<mavlink_message_t>("mavlink_message_t");
}

uint8_t MavLinkCommunicator::systemId() const
{
    return systemId_;
}

uint8_t MavLinkCommunicator::componentId() const
{
    return componentId_;
}

QList<AbstractLink*> MavLinkCommunicator::links() const
{
    return links_.keys();
}

void MavLinkCommunicator::setSystemId(const uint8_t systemId)
{
    systemId_ = systemId;
}

void MavLinkCommunicator::setComponentId(const uint8_t componentId)
{
    componentId_ = componentId;
}

void MavLinkCommunicator::sendMessage(const mavlink_message_t& message,
                                      AbstractLink* const link)
{
    if (link == nullptr || !link->isUp()) {
        return;
    }

    std::array<uint8_t, MAVLINK_MAX_PACKET_LEN> buffer{};
    const std::uint16_t length =
        mavlink_msg_to_send_buffer(buffer.data(), &message);
    link->sendData(reinterpret_cast<const char*>(buffer.data()), length);
}

void MavLinkCommunicator::sendMessageOnLastReceivedLink(
    const mavlink_message_t& message)
{
    sendMessage(message, lastReceivedLink_);
}

void MavLinkCommunicator::sendMessageOnMainLink(
    const mavlink_message_t& message)
{
    if (mainLink_ != nullptr) {
        sendMessage(message, mainLink_);
    }
}

void MavLinkCommunicator::sendMessageOnBackupLink(
    const mavlink_message_t& message)
{
    if (backupLink_ == nullptr) {
        return;
    }

    qDebug() << "send mav msgid:" << message.msgid << " to "
             << backupLink_->linkName_;
    sendMessage(message, backupLink_);
}

void MavLinkCommunicator::sendMessageOnAllLinks(
    const mavlink_message_t& message)
{
    std::array<uint8_t, MAVLINK_MAX_PACKET_LEN> buffer{};
    const std::uint16_t length =
        mavlink_msg_to_send_buffer(buffer.data(), &message);
    const QMap<AbstractLink*, LinkData> snapshot = links_;

    for (AbstractLink* const link : snapshot.keys()) {
        qDebug() << "send mav msgid:" << message.msgid << " to "
                 << link->linkName_;
        if (link->isUp()) {
            link->sendData(reinterpret_cast<const char*>(buffer.data()), length);
        }
    }
}

void MavLinkCommunicator::addLink(AbstractLink* const link,
                                  const uint8_t channel,
                                  const int id)
{
    if (links_.contains(link)) {
        return;
    }

    if (mainLink_ == nullptr) {
        mainLink_ = link;
    } else if (backupLink_ == nullptr) {
        backupLink_ = link;
    }

    links_.insert(
        link,
        LinkData{channel, static_cast<uint8_t>(id)});
    connect(link,
            &AbstractLink::dataReceived,
            this,
            &MavLinkCommunicator::onDataReceived);
}

void MavLinkCommunicator::removeLink(AbstractLink* const link)
{
    if (!links_.contains(link)) {
        return;
    }

    links_.remove(link);
    disconnect(link,
               &AbstractLink::dataReceived,
               this,
               &MavLinkCommunicator::onDataReceived);
}

void MavLinkCommunicator::onDataReceived(const QByteArray& data)
{
    AbstractLink* const link = qobject_cast<AbstractLink*>(sender());
    lastReceivedLink_ = link;
    if (link == nullptr) {
        return;
    }

    const qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (lastMessageTime_ > 0 && currentTime - lastMessageTime_ > 1000) {
        qDebug() << "mavlink msg:" << data.toHex();
    }

    const uint8_t channel = links_.value(link).channel;
    for (const char byte : data) {
        mavlink_message_t message{};
        mavlink_status_t status{};
        if (mavlink_parse_char(channel,
                               static_cast<uint8_t>(byte),
                               &message,
                               &status)
            == 0) {
            continue;
        }

        qDebug() << "msg seq:" << static_cast<int>(message.seq) << " id:"
                 << message.msgid;
        lastMessageTime_ = currentTime;
        if (link == mainLink_) {
            emit messageReceived(message);
        } else {
            emit backupMessageReceived(link->linkName_,
                                       message,
                                       links_.value(link).id);
        }
    }
}
