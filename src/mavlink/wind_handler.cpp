#include "mavlink/wind_handler.hpp"

#include <QDebug>
#include <QTimer>

WindHandler::WindHandler(MavLinkCommunicator* const communicator)
    : AbstractHandler(communicator)
{
    QTimer::singleShot(100, this, &WindHandler::initIntervals);
}

void WindHandler::initIntervals()
{
    setInterval(MAVLINK_MSG_ID_WIND, 10000.0F);
}

void WindHandler::processMessage(const mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_WIND) {
        return;
    }

    mavlink_wind_t wind{};
    mavlink_msg_wind_decode(&message, &wind);
    emit windEvent(wind.direction, wind.speed, wind.speed_z);
}

void WindHandler::backupProcessMessage(const QString& sender,
                                       const mavlink_message_t& message,
                                       const unsigned char linkId)
{
    qDebug() << "link:" << sender << " id:" << linkId;
    if (message.msgid != MAVLINK_MSG_ID_WIND) {
        return;
    }

    mavlink_wind_t wind{};
    mavlink_msg_wind_decode(&message, &wind);
    qDebug() << "backup link attitude..."
             << " wind_dir:" << wind.direction
             << " wind_speed:" << wind.speed
             << " wind_speed_z:" << wind.speed_z;
}
