#include "mavlink/abstract_handler.hpp"

#include <QDebug>

AbstractHandler::AbstractHandler(MavLinkCommunicator* const communicator)
    : QObject(communicator)
    , communicator_(communicator)
{
    connect(communicator_,
            &MavLinkCommunicator::messageReceived,
            this,
            &AbstractHandler::processMessage);
    connect(communicator_,
            &MavLinkCommunicator::backupMessageReceived,
            this,
            &AbstractHandler::backupProcessMessage);
}

void AbstractHandler::setName(const QString& name)
{
    name_ = name;
}

void AbstractHandler::setInterval(const int messageId,
                                  const float intervalMicroseconds)
{
    mavlink_message_t message{};
    const uint8_t systemId = communicator_->systemId();
    const uint8_t componentId = communicator_->componentId();
    mavlink_msg_command_long_pack(systemId,
                                  componentId,
                                  &message,
                                  systemId,
                                  componentId,
                                  MAV_CMD_SET_MESSAGE_INTERVAL,
                                  0,
                                  static_cast<float>(messageId),
                                  intervalMicroseconds,
                                  0.0F,
                                  0.0F,
                                  0.0F,
                                  0.0F,
                                  0.0F);
    qDebug() << name_ << "set interval" << messageId
             << intervalMicroseconds;
    communicator_->sendMessageOnAllLinks(message);
}

void AbstractHandler::backupProcessMessage(const QString&,
                                           const mavlink_message_t&,
                                           unsigned char)
{
}
