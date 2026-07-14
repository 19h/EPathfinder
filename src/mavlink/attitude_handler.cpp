#include "mavlink/attitude_handler.hpp"

#include <QDebug>
#include <QTimer>

namespace {
constexpr float kDegreesToRadians = 0.017453292519943295F;
}

AttitudeHandler::AttitudeHandler(
    MavLinkCommunicator* const communicator,
    const int attitudeIntervalMicroseconds)
    : AbstractHandler(communicator)
    , attitudeIntervalMicroseconds_(
          static_cast<float>(attitudeIntervalMicroseconds))
{
    QTimer::singleShot(100, this, &AttitudeHandler::initIntervals);
}

void AttitudeHandler::initIntervals()
{
    setInterval(MAVLINK_MSG_ID_ATTITUDE, attitudeIntervalMicroseconds_);
    setInterval(MAVLINK_MSG_ID_AHRS, 100.0F);
}

void AttitudeHandler::processMessage(const mavlink_message_t& message)
{
    if (message.msgid == MAVLINK_MSG_ID_ATTITUDE) {
        mavlink_attitude_t attitude{};
        mavlink_msg_attitude_decode(&message, &attitude);
        if (lastAttitudeTime_ == attitude.time_boot_ms) {
            qDebug() << "WTF: dupl att event";
            return;
        }
        lastAttitudeTime_ = attitude.time_boot_ms;
        emit attitudeEvent(attitude.time_boot_ms,
                           attitude.pitch,
                           attitude.yaw,
                           attitude.roll,
                           attitude.pitchspeed,
                           attitude.yawspeed,
                           attitude.rollspeed);
    } else if (message.msgid == MAVLINK_MSG_ID_AHRS) {
        mavlink_ahrs_t ahrs{};
        mavlink_msg_ahrs_decode(&message, &ahrs);
        qDebug() << "ahrs error_rp:" << ahrs.error_rp
                 << " error_yaw:" << ahrs.error_yaw;
        emit ahrsError(ahrs.error_rp, ahrs.error_yaw);
    }
}

void AttitudeHandler::backupProcessMessage(const QString& sender,
                                           const mavlink_message_t& message,
                                           const unsigned char linkId)
{
    qDebug() << "link:" << sender << " id:" << linkId;
    if (message.msgid != MAVLINK_MSG_ID_ATTITUDE) {
        return;
    }

    mavlink_attitude_t attitude{};
    mavlink_msg_attitude_decode(&message, &attitude);
    qDebug() << "backup link attitude..."
             << " time:" << attitude.time_boot_ms
             << " pitch:" << attitude.pitch
             << " yaw:" << attitude.yaw
             << " roll:" << attitude.roll
             << " pitchSpeed:" << attitude.pitchspeed
             << " yawSpeed:" << attitude.yawspeed
             << " rollSpeed:" << attitude.rollspeed;
    emit backupAttitudeEvent(linkId,
                             attitude.time_boot_ms,
                             attitude.pitch,
                             attitude.yaw,
                             attitude.roll,
                             attitude.pitchspeed,
                             attitude.yawspeed,
                             attitude.rollspeed);
}

void AttitudeHandler::setAHRS(const qlonglong time,
                              const float roll,
                              const float pitch,
                              const float yaw)
{
    qDebug() << "setAHRS time, roll, pitch, yaw:"
             << time << roll << pitch << yaw;
    mavlink_message_t message{};
    const uint8_t systemId = communicator_->systemId();
    mavlink_msg_attitude_pack(systemId,
                              MAV_COMP_ID_ONBOARD_COMPUTER,
                              &message,
                              static_cast<uint32_t>(time),
                              roll * kDegreesToRadians,
                              pitch * kDegreesToRadians,
                              yaw * kDegreesToRadians,
                              0.0F,
                              0.0F,
                              0.0F);
    communicator_->sendMessageOnAllLinks(message);
}
