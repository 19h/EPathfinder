#include "mavlink/gps_handler.hpp"

#include <QDebug>
#include <QTimer>

GpsHandler::GpsHandler(MavLinkCommunicator* const communicator)
    : AbstractHandler(communicator)
{
    QTimer::singleShot(100, this, &GpsHandler::initIntervals);
}

uint8_t GpsHandler::satelliteCount() const
{
    return satelliteCount_;
}

void GpsHandler::initIntervals()
{
    setInterval(MAVLINK_MSG_ID_GLOBAL_POSITION_INT, 10000.0F);
    setInterval(MAVLINK_MSG_ID_GPS_RAW_INT, 10000.0F);
}

void GpsHandler::processMessage(const mavlink_message_t& message)
{
    if (message.msgid == MAVLINK_MSG_ID_GPS_RAW_INT) {
        mavlink_gps_raw_int_t raw{};
        mavlink_msg_gps_raw_int_decode(&message, &raw);
        qDebug() << "RAW_GPS"
                 << " time_usec:" << raw.time_usec
                 << " lat:" << raw.lat
                 << " lon:" << raw.lon
                 << " alt:" << raw.alt
                 << " eph:" << raw.eph
                 << " epv:" << raw.epv
                 << " vel:" << raw.vel
                 << " cog:" << raw.cog
                 << " fix_type:" << raw.fix_type
                 << " satellites_visible:" << raw.satellites_visible
                 << " alt_ellipsoid:" << raw.alt_ellipsoid
                 << " h_acc:" << raw.h_acc
                 << " v_acc:" << raw.v_acc
                 << " vel_acc:" << raw.vel_acc
                 << " hdg_acc:" << raw.hdg_acc
                 << " yaw:" << raw.yaw;
        emit rawPositionEvent(static_cast<uint32_t>(raw.time_usec / 1000U),
                              raw.lat,
                              raw.lon,
                              raw.alt);
        if (!satelliteCountInitialized_
            || satelliteCount_ != raw.satellites_visible) {
            qDebug() << "satellite count:" << raw.satellites_visible;
            satelliteCountInitialized_ = true;
            satelliteCount_ = raw.satellites_visible;
            emit satelliteCountChanged();
        }
        return;
    }

    if (message.msgid == MAVLINK_MSG_ID_GLOBAL_POSITION_INT) {
        mavlink_global_position_int_t position{};
        mavlink_msg_global_position_int_decode(&message, &position);
        emit positionEvent(position.time_boot_ms,
                           position.lat,
                           position.lon,
                           position.alt,
                           position.relative_alt,
                           position.vx,
                           position.vy,
                           position.vz,
                           position.hdg);
    }
}

void GpsHandler::backupProcessMessage(const QString& sender,
                                      const mavlink_message_t& message,
                                      const unsigned char linkId)
{
    qDebug() << "link:" << sender << " linkId:" << linkId
             << " msgId:" << message.msgid;
    if (message.msgid == MAVLINK_MSG_ID_GPS_RAW_INT) {
        mavlink_gps_raw_int_t raw{};
        mavlink_msg_gps_raw_int_decode(&message, &raw);
        if (satelliteCount_ != raw.satellites_visible) {
            qDebug() << "backup link satellite count:"
                     << raw.satellites_visible;
        }
    } else if (message.msgid == MAVLINK_MSG_ID_GLOBAL_POSITION_INT) {
        mavlink_global_position_int_t position{};
        mavlink_msg_global_position_int_decode(&message, &position);
        qDebug() << "backup link position..."
                 << " time:" << position.time_boot_ms
                 << " lat:" << position.lat
                 << " lon:" << position.lon
                 << " alt:" << position.alt
                 << " mslAlt:" << position.relative_alt
                 << " vx:" << position.vx
                 << " vy:" << position.vy
                 << " vz:" << position.vz;
    }
}
