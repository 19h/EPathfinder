#include "mavlink/rc_channels_handler.hpp"

#include <QDebug>
#include <QTimer>

RcChannelsHandler::RcChannelsHandler(MavLinkCommunicator* const communicator)
    : AbstractHandler(communicator)
{
    QTimer::singleShot(100, this, &RcChannelsHandler::initIntervals);
}

void RcChannelsHandler::initIntervals()
{
    setInterval(MAVLINK_MSG_ID_RC_CHANNELS, 10000.0F);
}

void RcChannelsHandler::updateHoming(const uint16_t value, const bool backup)
{
    bool& state = backup ? backupHoming_ : homing_;
    if (value > homingOnThreshold_) {
        if (!state) {
            state = true;
            backup ? emit backupHomingOn() : emit homingOn();
        }
    } else if (value < homingOffThreshold_ && state) {
        state = false;
        backup ? emit backupHomingOff() : emit homingOff();
    }
}

void RcChannelsHandler::processMessage(const mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_RC_CHANNELS) {
        return;
    }

    mavlink_rc_channels_t channels{};
    mavlink_msg_rc_channels_decode(&message, &channels);
    emit currentControls(channels.chan1_raw,
                         static_cast<uint16_t>(3000U - channels.chan2_raw),
                         channels.chan3_raw);
    updateHoming(channels.chan10_raw, false);
    emit currentChannels(channels.chan5_raw,
                         channels.chan6_raw,
                         channels.chan7_raw,
                         channels.chan8_raw,
                         channels.chan9_raw,
                         channels.chan10_raw,
                         channels.chan11_raw,
                         channels.chan12_raw,
                         channels.chan13_raw,
                         channels.chan14_raw,
                         channels.chan15_raw,
                         channels.chan16_raw,
                         channels.chan17_raw,
                         channels.chan18_raw);
}

void RcChannelsHandler::backupProcessMessage(const QString& sender,
                                             const mavlink_message_t& message,
                                             const unsigned char linkId)
{
    qDebug() << "link:" << sender << " linkId:" << linkId
             << " msgId:" << message.msgid;
    if (message.msgid != MAVLINK_MSG_ID_RC_CHANNELS) {
        return;
    }

    mavlink_rc_channels_t channels{};
    mavlink_msg_rc_channels_decode(&message, &channels);
    updateHoming(channels.chan10_raw, true);
}
