#include "mavlink/control_handler.hpp"

#include <QByteArray>
#include <QDateTime>
#include <QTimerEvent>

#include <algorithm>
#include <array>
#include <cstring>

namespace {

constexpr std::uint8_t kCommandSourceSystem = 1U;
constexpr std::uint8_t kOnboardComputerComponent = 191U;
constexpr std::uint8_t kAutopilotComponent = 1U;
constexpr std::uint8_t kMountComponent = 154U;
constexpr std::uint8_t kOverrideSourceSystem = 255U;
constexpr qint64 kBackupDropDurationMs = 1000;

constexpr std::uint16_t kStartMagCalCommand = 42424U;
constexpr std::uint16_t kAcceptMagCalCommand = 42425U;
constexpr std::uint16_t kCancelMagCalCommand = 42426U;

std::uint16_t controlValue(const int value)
{
    return static_cast<std::uint16_t>(std::clamp(value, 1000, 2000));
}

} // namespace

ControlHandler::ControlHandler(MavLinkCommunicator* const communicator,
                               const int interval)
    : AbstractHandler(communicator)
{
    startTimer(interval, Qt::CoarseTimer);
}

unsigned char ControlHandler::currentMountControlCenterOnValue() const
{
    return mountControlCenterOnValue_;
}

unsigned char ControlHandler::currentMountControlCenterOffValue() const
{
    return mountControlCenterOffValue_;
}

void ControlHandler::initIntervals()
{
}

void ControlHandler::setControlParams(const int roll,
                                      const int yaw,
                                      const int pitch,
                                      const int throttle)
{
    roll_ = controlValue(roll);
    yaw_ = controlValue(yaw);
    pitch_ = controlValue(pitch);
    throttle_ = controlValue(throttle);
}

void ControlHandler::setFlapsEnabled(const bool enabled)
{
    flapsEnabled_ = enabled;
}

void ControlHandler::setFlapsValue(const int value)
{
    flapsValue_ = static_cast<std::uint16_t>(value);
}

void ControlHandler::controlOff()
{
    controlEnabled_ = false;
    if (reverseThrottleEnabled_) {
        reverseThrottle_ = false;
    }

    roll_ = 0U;
    yaw_ = 0U;
    pitch_ = 0U;
    throttle_ = 0U;

    sendPrimaryOverride(0U,
                        0U,
                        0U,
                        0U,
                        0U,
                        ptzCameraEnabled_ ? ptzValue_ : 0U,
                        0U);
    if (backupControllerEnabled_) {
        sendBackupOverride(0U, 0U, 0U, 0U, 0U);
    }
}

void ControlHandler::setCameraParams(const int pitch)
{
    Q_UNUSED(pitch)
}

void ControlHandler::setParachuteEnabled(const bool enabled)
{
    parachuteEnabled_ = enabled;
    if (!enabled || controlEnabled_) {
        return;
    }

    sendPrimaryOverride(0U, 0U, 0U, 0U, 1000U, 0U, 0U);
}

void ControlHandler::setPTZCameraEnabled(const bool enabled)
{
    ptzCameraEnabled_ = enabled;
}

void ControlHandler::setMinelayerEnabled(const bool enabled)
{
    minelayerEnabled_ = enabled;
}

void ControlHandler::parachuteOn()
{
    parachuteValue_ = 2000U;
}

void ControlHandler::ptzCameraOn()
{
    ptzValue_ = 1000U;
}

void ControlHandler::ptzCameraOff()
{
    ptzValue_ = 2000U;
}

void ControlHandler::setThrottleMinValue(const unsigned short value)
{
    throttleMin_ = value;
}

void ControlHandler::setMountControlCenterOnValue(const unsigned char value)
{
    mountControlCenterOnValue_ = value;
}

void ControlHandler::setMountControlCenterOffValue(const unsigned char value)
{
    mountControlCenterOffValue_ = value;
}

void ControlHandler::enableRpmControl(const bool enable)
{
    rpmControlEnabled_ = enable;
}

void ControlHandler::startRpmControl()
{
    rpmControlActive_ = true;
}

void ControlHandler::stopRpmControl()
{
    rpmControlActive_ = false;
}

void ControlHandler::enableSpyCamera(const bool enabled)
{
    spyCameraEnabled_ = enabled;
}

void ControlHandler::openSpyCamera()
{
    spyCameraOpen_ = true;
}

void ControlHandler::closeSpyCamera()
{
    spyCameraOpen_ = false;
}

void ControlHandler::setBackupControllerEnabled(const bool value)
{
    backupControllerEnabled_ = value;
}

void ControlHandler::setBackupSystemId(const int value)
{
    backupSystemId_ = static_cast<unsigned char>(value);
}

void ControlHandler::setBackupControllerInvertPitch(const bool isInvert)
{
    backupControllerInvertPitch_ = isInvert;
}

void ControlHandler::setBackupControllerDropEnabled(const bool enabled)
{
    backupControllerDropEnabled_ = enabled;
}

void ControlHandler::startBackupControllerDrop()
{
    backupControllerDropStartTime_ = QDateTime::currentMSecsSinceEpoch();
}

void ControlHandler::dropMinelayer()
{
    minelayerValue_ = 2000U;
}

void ControlHandler::setReverseThrottleEnabled(const bool enabled)
{
    reverseThrottleEnabled_ = enabled;
}

void ControlHandler::setNormalThrottle()
{
    reverseThrottle_ = false;
}

void ControlHandler::setReverseThrottle()
{
    reverseThrottle_ = true;
}

void ControlHandler::setEPTZ(const float roll,
                             const float pitch,
                             const float yaw,
                             const float rollMaxSpeed,
                             const float pitchMaxSpeed,
                             const float yawMaxSpeed,
                             const bool center,
                             const bool yawByNorth,
                             const unsigned char gimbalId)
{
    eptzCenter_ = center;
    eptzRoll_ = roll;
    eptzPitch_ = pitch;
    eptzYaw_ = yaw;

    if (eptzCommandSuppressed_) {
        return;
    }

    float p1 = pitch;
    float p2 = roll;
    float p3 = yaw;
    float p4 = 0.0F;
    float p5 = 0.0F;
    float p6 = 0.0F;
    float p7 = static_cast<float>(center ? mountControlCenterOnValue_
                                         : mountControlCenterOffValue_);

    if (gimbalId == 172U || gimbalId == 173U) {
        p1 = yaw;
        p2 = roll;
        p3 = pitch;
        p4 = yawMaxSpeed;
        p5 = rollMaxSpeed;
        p6 = pitchMaxSpeed;
        p7 = center ? 3.0F : (yawByNorth ? 2.0F : 1.0F);
    }

    mavlink_message_t message{};
    mavlink_msg_command_long_pack(kCommandSourceSystem,
                                  kOnboardComputerComponent,
                                  &message,
                                  kCommandSourceSystem,
                                  gimbalId,
                                  MAV_CMD_DO_MOUNT_CONTROL,
                                  0U,
                                  p1,
                                  p2,
                                  p3,
                                  p4,
                                  p5,
                                  p6,
                                  p7);
    communicator_->sendMessageOnMainLink(message);
}

void ControlHandler::writeParam(const QString& name, const int value)
{
    const QByteArray bytes = name.toUtf8();
    std::array<char, 16> parameterName{};
    const qsizetype copyLength = std::min<qsizetype>(bytes.size(), 15);
    if (copyLength > 0) {
        std::memcpy(parameterName.data(), bytes.constData(),
                    static_cast<std::size_t>(copyLength));
    }

    mavlink_message_t message{};
    mavlink_msg_param_set_pack(communicator_->systemId(),
                               kOnboardComputerComponent,
                               &message,
                               communicator_->systemId(),
                               communicator_->componentId(),
                               parameterName.data(),
                               static_cast<float>(value),
                               MAV_PARAM_TYPE_INT16);
    communicator_->sendMessageOnMainLink(message);
}

void ControlHandler::mavCmdDoSet(const uint16_t command,
                                 const float p1,
                                 const float p2,
                                 const float p3,
                                 const float p4,
                                 const float p5,
                                 const float p6,
                                 const float p7)
{
    mavlink_message_t message{};
    mavlink_msg_command_long_pack(
        kCommandSourceSystem,
        kOnboardComputerComponent,
        &message,
        kCommandSourceSystem,
        command == MAV_CMD_DO_MOUNT_CONTROL ? kMountComponent
                                            : kAutopilotComponent,
        command,
        0U,
        p1,
        p2,
        p3,
        p4,
        p5,
        p6,
        p7);

    if (command == MAV_CMD_DO_SET_MODE && backupControllerEnabled_) {
        communicator_->sendMessageOnAllLinks(message);
    } else {
        communicator_->sendMessageOnMainLink(message);
    }
}

void ControlHandler::setRelay(const int id, const bool value)
{
    mavCmdDoSet(MAV_CMD_DO_SET_RELAY,
                static_cast<float>(id),
                value ? 1.0F : 0.0F);
}

void ControlHandler::cameraOn()
{
    setRelay(2, true);
}

void ControlHandler::cameraOff()
{
    setRelay(2, false);
}

void ControlHandler::burnRocket()
{
    setRelay(1, true);
}

void ControlHandler::takeoff()
{
    mavCmdDoSet(MAV_CMD_DO_SET_MODE, 1.0F, 13.0F);
}

void ControlHandler::arm(const bool value)
{
    mavCmdDoSet(MAV_CMD_COMPONENT_ARM_DISARM,
                value ? 1.0F : 0.0F,
                value ? 2989.0F : 21196.0F);
}

void ControlHandler::setMode(const int mode)
{
    mavCmdDoSet(MAV_CMD_DO_SET_MODE,
                1.0F,
                static_cast<float>(mode),
                1.0F);
}

void ControlHandler::controlOn(const int mode)
{
    controlEnabled_ = true;
    setMode(mode);
}

void ControlHandler::separateRocketStage()
{
    mavCmdDoSet(MAV_CMD_DO_SET_SERVO, 1.0F, 2000.0F);
}

void ControlHandler::reboot()
{
    mavCmdDoSet(MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN, 1.0F, 1.0F, 1.0F);
}

void ControlHandler::startMagCal()
{
    mavCmdDoSet(kStartMagCalCommand, 0.0F, 1.0F, 1.0F);
}

void ControlHandler::acceptMagCal()
{
    mavCmdDoSet(kAcceptMagCalCommand);
}

void ControlHandler::cancelMagCal()
{
    mavCmdDoSet(kCancelMagCalCommand);
}

void ControlHandler::startLevelCalibration()
{
    mavCmdDoSet(MAV_CMD_PREFLIGHT_CALIBRATION,
                0.0F,
                0.0F,
                0.0F,
                0.0F,
                2.0F);
}

void ControlHandler::processMessage(const mavlink_message_t& message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_COMMAND_ACK: {
        mavlink_command_ack_t ack{};
        mavlink_msg_command_ack_decode(&message, &ack);
        switch (ack.command) {
        case kStartMagCalCommand:
            emit startMagCalAck(ack.result);
            break;
        case kAcceptMagCalCommand:
            emit acceptMagCalAck(ack.result);
            break;
        case kCancelMagCalCommand:
            emit cancelMagCalAck(ack.result);
            break;
        case MAV_CMD_PREFLIGHT_CALIBRATION:
            emit levelCalAck(ack.result);
            break;
        default:
            break;
        }
        break;
    }
    case MAVLINK_MSG_ID_MAG_CAL_PROGRESS: {
        mavlink_mag_cal_progress_t progress{};
        mavlink_msg_mag_cal_progress_decode(&message, &progress);
        magCalCompletion_ = progress.completion_pct;
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - lastMagCalCompletionTime_ >= magCalCompletionInterval_
            || magCalCompletion_ > 99U) {
            magCalCompletionPending_ = false;
            lastMagCalCompletionTime_ = now;
            emit magCalCompletionPercentage(magCalCompletion_);
        } else {
            magCalCompletionPending_ = true;
        }
        break;
    }
    case MAVLINK_MSG_ID_MAG_CAL_REPORT: {
        mavlink_mag_cal_report_t report{};
        mavlink_msg_mag_cal_report_decode(&message, &report);
        magCalStatusValue_ = report.cal_status;
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - lastMagCalStatusTime_ >= magCalStatusInterval_) {
            magCalStatusPending_ = false;
            lastMagCalStatusTime_ = now;
            emit magCalStatus(magCalStatusValue_);
        } else {
            magCalStatusPending_ = true;
        }
        break;
    }
    case MAVLINK_MSG_ID_RPM: {
        mavlink_rpm_t rpm{};
        mavlink_msg_rpm_decode(&message, &rpm);
        emit rpmValues(static_cast<int>(rpm.rpm1),
                       static_cast<int>(rpm.rpm2));
        break;
    }
    default:
        break;
    }
}

void ControlHandler::sendPrimaryOverride(const std::uint16_t chan1,
                                         const std::uint16_t chan2,
                                         const std::uint16_t chan3,
                                         const std::uint16_t chan4,
                                         const std::uint16_t chan6,
                                         const std::uint16_t chan7,
                                         const std::uint16_t chan12)
{
    mavlink_message_t message{};
    mavlink_msg_rc_channels_override_pack(
        kOverrideSourceSystem,
        kOnboardComputerComponent,
        &message,
        communicator_->systemId(),
        communicator_->componentId(),
        chan1,
        chan2,
        chan3,
        chan4,
        0U,
        chan6,
        chan7,
        0U,
        0U,
        0U,
        0U,
        chan12,
        0U,
        0U,
        0U,
        0U,
        0U,
        0U);
    communicator_->sendMessageOnMainLink(message);
}

void ControlHandler::sendBackupOverride(const std::uint16_t chan1,
                                        const std::uint16_t chan2,
                                        const std::uint16_t chan3,
                                        const std::uint16_t chan4,
                                        const std::uint16_t chan12)
{
    mavlink_message_t message{};
    mavlink_msg_rc_channels_override_pack(
        backupSystemId_,
        kOnboardComputerComponent,
        &message,
        communicator_->systemId(),
        communicator_->componentId(),
        chan1,
        chan2,
        chan3,
        chan4,
        0U,
        0U,
        0U,
        0U,
        0U,
        0U,
        0U,
        chan12,
        0U,
        0U,
        0U,
        0U,
        0U,
        0U);
    communicator_->sendMessageOnBackupLink(message);
}

void ControlHandler::sendChanValues()
{
    std::uint16_t chan1 = 0U;
    std::uint16_t chan2 = 0U;
    std::uint16_t chan3 = 0U;
    std::uint16_t chan4 = 0U;
    std::uint16_t chan6 = 0U;
    std::uint16_t chan12 = 0U;

    if (controlEnabled_) {
        chan1 = roll_;
        chan2 = static_cast<std::uint16_t>(3000U - pitch_);
        chan3 = throttle_ == 1000U ? throttleMin_ : throttle_;
        chan4 = yaw_;

        if (rpmControlEnabled_) {
            chan6 = rpmControlActive_ ? 2000U : 1000U;
        }
        if (parachuteEnabled_) {
            chan6 = parachuteValue_;
        }
        if (reverseThrottleEnabled_) {
            chan6 = reverseThrottle_ ? 1000U : 2000U;
        }
        if (flapsEnabled_) {
            chan6 = flapsValue_;
        }

        if (minelayerEnabled_) {
            chan12 = minelayerValue_;
        }
        if (spyCameraEnabled_) {
            chan12 = spyCameraOpen_ ? 2000U : 1000U;
        }
    }

    sendPrimaryOverride(chan1,
                        chan2,
                        chan3,
                        chan4,
                        chan6,
                        ptzCameraEnabled_ ? ptzValue_ : 0U,
                        chan12);

    if (!backupControllerEnabled_) {
        return;
    }

    std::uint16_t backupRoll = 0U;
    std::uint16_t backupPitch = 0U;
    std::uint16_t backupThrottle = 0U;
    std::uint16_t backupYaw = 0U;
    std::uint16_t backupChan12 = 0U;
    if (controlEnabled_) {
        backupRoll = roll_;
        backupPitch = backupControllerInvertPitch_
            ? static_cast<std::uint16_t>(3000U - pitch_)
            : pitch_;
        backupThrottle = throttle_;
        backupYaw = yaw_;
        if (backupControllerDropEnabled_) {
            const qint64 elapsed = QDateTime::currentMSecsSinceEpoch()
                - backupControllerDropStartTime_;
            backupChan12 = elapsed > (kBackupDropDurationMs - 1)
                ? 1000U
                : 2000U;
        }
    }
    sendBackupOverride(backupRoll,
                       backupPitch,
                       backupThrottle,
                       backupYaw,
                       backupChan12);
}

void ControlHandler::timerEvent(QTimerEvent* const event)
{
    Q_UNUSED(event)

    if (controlEnabled_ || ptzCameraEnabled_) {
        sendChanValues();
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (magCalCompletionPending_
        && now - lastMagCalCompletionTime_ > magCalCompletionInterval_) {
        lastMagCalCompletionTime_ = now;
        emit magCalCompletionPercentage(magCalCompletion_);
        magCalCompletionPending_ = false;
    }
    if (magCalStatusPending_
        && now - lastMagCalStatusTime_ > magCalStatusInterval_) {
        lastMagCalStatusTime_ = now;
        emit magCalStatus(magCalStatusValue_);
        magCalStatusPending_ = false;
    }
}
