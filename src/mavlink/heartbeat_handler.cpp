#include "mavlink/heartbeat_handler.hpp"

#include <QByteArray>
#include <QDebug>
#include <QTimer>
#include <QTimerEvent>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace {

QString fixedString(const char* const data, const int capacity)
{
    int length = 0;
    while (length < capacity && data[length] != '\0') {
        ++length;
    }
    return QString::fromLatin1(data, length);
}

QString fixedString(const std::uint8_t* const data, const int capacity)
{
    return fixedString(reinterpret_cast<const char*>(data), capacity);
}

std::array<char, 16> parameterId(const QString& name)
{
    std::array<char, 16> result{};
    const QByteArray bytes = name.toLocal8Bit();
    const auto length = std::min<std::size_t>(
        result.size(), static_cast<std::size_t>(bytes.size()));
    std::memcpy(result.data(), bytes.constData(), length);
    return result;
}

bool supportedVehicleType(const std::uint8_t type)
{
    return type == MAV_TYPE_FIXED_WING || type == MAV_TYPE_QUADROTOR
        || type == MAV_TYPE_HEXAROTOR || type == MAV_TYPE_OCTOROTOR;
}

} // namespace

HeartbeatHandler::HeartbeatHandler(const unsigned char id,
                                   MavLinkCommunicator* const communicator,
                                   const float airspeedRatio)
    : AbstractHandler(communicator)
    , id_(id)
    , airspeedRatio_(airspeedRatio)
{
    QTimer::singleShot(100, this, &HeartbeatHandler::initIntervals);
    QTimer::singleShot(500, this, &HeartbeatHandler::sendOsdMessage);
    QTimer::singleShot(200, this, &HeartbeatHandler::writeParams);
    QTimer::singleShot(1000, this, &HeartbeatHandler::readParams);
    QTimer::singleShot(100, this, &HeartbeatHandler::readArdupilotVersion);
    startTimer(500, Qt::CoarseTimer);
}

void HeartbeatHandler::readAcceleration(const bool enabled)
{
    readAcceleration_ = enabled;
    if (enabled) {
        setInterval(MAVLINK_MSG_ID_RAW_IMU, 20000.0F);
    }
}

double HeartbeatHandler::lastAcceleration() const
{
    return std::sqrt(static_cast<double>(lastAccelerationSquared_));
}

std::uint16_t HeartbeatHandler::lastFlowXValue() const
{
    return lastFlowX_;
}

std::uint16_t HeartbeatHandler::lastFlowYValue() const
{
    return lastFlowY_;
}

void HeartbeatHandler::setRocketLaunch(const bool enabled)
{
    rocketLaunch_ = enabled;
}

void HeartbeatHandler::setAccDelay(const unsigned int milliseconds)
{
    accelerationDelayMs_ = milliseconds;
}

void HeartbeatHandler::rawIMULastValues(qlonglong& time,
                                        short& xacc,
                                        short& yacc,
                                        short& zacc,
                                        short& xgyro,
                                        short& ygyro,
                                        short& zgyro,
                                        short& xmag,
                                        short& ymag,
                                        short& zmag) const
{
    time = rawImuTime_;
    xacc = rawXacc_;
    yacc = rawYacc_;
    zacc = rawZacc_;
    xgyro = rawXgyro_;
    ygyro = rawYgyro_;
    zgyro = rawZgyro_;
    xmag = rawXmag_;
    ymag = rawYmag_;
    zmag = rawZmag_;
}

void HeartbeatHandler::disableOtherMessages()
{
    qDebug() << "HeartbeatHandler::disableOtherMessages";
    setInterval(MAVLINK_MSG_ID_SCALED_IMU, 500000.0F);
    setInterval(MAVLINK_MSG_ID_SCALED_PRESSURE, 500000.0F);
    setInterval(MAVLINK_MSG_ID_COMMAND_ACK, 500000.0F);
}

void HeartbeatHandler::initIntervals()
{
    setInterval(MAVLINK_MSG_ID_HEARTBEAT, 100000.0F);
    setInterval(MAVLINK_MSG_ID_SCALED_IMU, 500000.0F);
    setInterval(MAVLINK_MSG_ID_RAW_IMU, 20000.0F);
    setInterval(MAVLINK_MSG_ID_SYS_STATUS, 500000.0F);
    setInterval(MAVLINK_MSG_ID_VFR_HUD, 50000.0F);
    setInterval(MAVLINK_MSG_ID_SCALED_PRESSURE, 500000.0F);
    setInterval(MAVLINK_MSG_ID_DISTANCE_SENSOR, 50000.0F);
    setInterval(MAVLINK_MSG_ID_OPTICAL_FLOW, 20000.0F);
}

void HeartbeatHandler::backupProcessMessage(const QString& sender,
                                            const mavlink_message_t& message,
                                            const unsigned char)
{
    qDebug() << "link:" << sender << " id:" << message.msgid;

    switch (message.msgid) {
    case MAVLINK_MSG_ID_HEARTBEAT: {
        mavlink_heartbeat_t heartbeat{};
        mavlink_msg_heartbeat_decode(&message, &heartbeat);
        if (supportedVehicleType(heartbeat.type)
            && backupCurrentMode_ != static_cast<int>(heartbeat.custom_mode)) {
            qDebug() << "backup current mode changed from:"
                     << backupCurrentMode_ << " to:" << heartbeat.custom_mode;
            backupCurrentMode_ = static_cast<int>(heartbeat.custom_mode);
        }
        break;
    }
    case MAVLINK_MSG_ID_VFR_HUD: {
        mavlink_vfr_hud_t hud{};
        mavlink_msg_vfr_hud_decode(&message, &hud);
        qDebug() << "backup VFR_HUD airspeed:" << hud.airspeed
                 << " groundspeed:" << hud.groundspeed
                 << " alt:" << hud.alt << " heading:" << hud.heading
                 << " climb:" << hud.climb << " throttle:" << hud.throttle;
        break;
    }
    case MAVLINK_MSG_ID_RAW_IMU: {
        mavlink_raw_imu_t raw{};
        mavlink_msg_raw_imu_decode(&message, &raw);
        qDebug() << "backup link RAW_IMU time:" << raw.time_usec
                 << " xacc:" << raw.xacc << " yacc:" << raw.yacc
                 << " zacc:" << raw.zacc << " xgyro:" << raw.xgyro
                 << " ygyro:" << raw.ygyro << " zgyro:" << raw.zgyro
                 << " xmag:" << raw.xmag << " ymag:" << raw.ymag
                 << " zmag:" << raw.zmag << " id:" << raw.id
                 << " temperature:" << raw.temperature;
        break;
    }
    case MAVLINK_MSG_ID_SCALED_IMU: {
        mavlink_scaled_imu_t scaled{};
        mavlink_msg_scaled_imu_decode(&message, &scaled);
        qDebug() << "backup link SCALED_IMU time:" << scaled.time_boot_ms
                 << " xacc:" << scaled.xacc << " yacc:" << scaled.yacc
                 << " zacc:" << scaled.zacc << " xgyro:" << scaled.xgyro
                 << " ygyro:" << scaled.ygyro << " zgyro:" << scaled.zgyro
                 << " xmag:" << scaled.xmag << " ymag:" << scaled.ymag
                 << " zmag:" << scaled.zmag
                 << " temperature:" << scaled.temperature;
        break;
    }
    case MAVLINK_MSG_ID_SCALED_PRESSURE: {
        mavlink_scaled_pressure_t pressure{};
        mavlink_msg_scaled_pressure_decode(&message, &pressure);
        qDebug() << "backup link SCALED_PRESSURE time:"
                 << pressure.time_boot_ms << " press_abs:"
                 << pressure.press_abs << " press_diff:"
                 << pressure.press_diff << " temperatue:"
                 << pressure.temperature << " temperature_press_diff:"
                 << pressure.temperature_press_diff;
        break;
    }
    case MAVLINK_MSG_ID_BATTERY_STATUS: {
        mavlink_battery_status_t battery{};
        mavlink_msg_battery_status_decode(&message, &battery);
        qDebug() << "backup link BATTERY_STATUS id:" << battery.id
                 << " current_battery:" << battery.current_battery;
        emit backupBattery(battery.id, battery.current_battery);
        break;
    }
    default:
        break;
    }
}

void HeartbeatHandler::setManualAccelerationEvent()
{
    lastAccelerationSquared_ = 25000001;
    qDebug() << "manual acceleration 5G:" << lastAcceleration();
    QTimer::singleShot(static_cast<int>(accelerationDelayMs_),
                       this,
                       &HeartbeatHandler::launchAcceleration);
    setInterval(MAVLINK_MSG_ID_RAW_IMU, 1000000.0F);
    accelerationEventSent_ = true;
}

void HeartbeatHandler::showMessageOnOSD(const QString&)
{
}

void HeartbeatHandler::sendOsdMessage()
{
}

void HeartbeatHandler::enableAccelLog()
{
    accelerationLog_ = true;
    setInterval(MAVLINK_MSG_ID_RAW_IMU, 20000.0F);
}

void HeartbeatHandler::disableAccelLog()
{
    accelerationLog_ = false;
    setInterval(MAVLINK_MSG_ID_RAW_IMU, 1000000.0F);
}

void HeartbeatHandler::readArdupilotVersion()
{
    if (!ardupilotVersion_.isEmpty()) {
        return;
    }

    mavlink_message_t message{};
    const std::uint8_t systemId = communicator_->systemId();
    const std::uint8_t componentId = communicator_->componentId();
    mavlink_msg_command_long_pack(systemId,
                                  MAV_COMP_ID_ONBOARD_COMPUTER,
                                  &message,
                                  systemId,
                                  componentId,
                                  MAV_CMD_REQUEST_MESSAGE,
                                  0,
                                  static_cast<float>(
                                      MAVLINK_MSG_ID_AUTOPILOT_VERSION),
                                  0.0F,
                                  0.0F,
                                  0.0F,
                                  0.0F,
                                  0.0F,
                                  0.0F);
    communicator_->sendMessageOnMainLink(message);
    QTimer::singleShot(1000, this, &HeartbeatHandler::readArdupilotVersion);
}

void HeartbeatHandler::readParam(const QString& name)
{
    qDebug() << "request read param:" << name;
    if (!pendingParameters_.contains(name)) {
        qDebug() << "add wait param:" << name;
        pendingParameters_.insert(name);
    }

    const auto id = parameterId(name);
    mavlink_message_t message{};
    const std::uint8_t systemId = communicator_->systemId();
    mavlink_msg_param_request_read_pack(systemId,
                                        MAV_COMP_ID_ONBOARD_COMPUTER,
                                        &message,
                                        systemId,
                                        communicator_->componentId(),
                                        id.data(),
                                        -1);
    communicator_->sendMessageOnAllLinks(message);
}

void HeartbeatHandler::timerEvent(QTimerEvent*)
{
    const QSet<QString> pending = pendingParameters_;
    for (const QString& name : pending) {
        readParam(name);
    }
}

void HeartbeatHandler::readParams()
{
    static const std::array<const char*, 10> names{{
        "SERVO3_MAX",
        "SERVO3_MIN",
        "SERVO3_TRIM",
        "SERVO4_MAX",
        "SERVO4_MIN",
        "SERVO4_TRIM",
        "SERVO1_MAX",
        "SERVO1_MIN",
        "SERVO10_TRIM",
        "RC3_MIN",
    }};
    for (const char* const name : names) {
        readParam(QString::fromLatin1(name));
    }
}

void HeartbeatHandler::writeParam(const QString& name, const float value)
{
    qDebug() << "write param:" << name << " value:" << value;
    writtenParameters_[name] = value;

    const auto id = parameterId(name);
    mavlink_message_t message{};
    const std::uint8_t systemId = communicator_->systemId();
    mavlink_msg_param_set_pack(systemId,
                               MAV_COMP_ID_ONBOARD_COMPUTER,
                               &message,
                               systemId,
                               communicator_->componentId(),
                               id.data(),
                               value,
                               MAV_PARAM_TYPE_INT16);
    communicator_->sendMessageOnMainLink(message);
    QTimer::singleShot(parameterReadDelayMs_, this, [this, name] {
        readParam(name);
    });
}

void HeartbeatHandler::writeParams()
{
    writeParam(QStringLiteral("MNT1_LEAD_PTCH"), 0.03F);
    writeParam(QStringLiteral("ARSPD_RATIO"), airspeedRatio_);
    writeParam(QStringLiteral("PILOT_SPEED_UP"), 1000.0F);
    writeParam(QStringLiteral("PILOT_SPEED_DN"), 1000.0F);
    writeParam(QStringLiteral("COMPASS_CAL_FIT"), 4.0F);
    writeParam(QStringLiteral("COMPASS_AUTO_ROT"), 1.0F);
    writeParam(QStringLiteral("MNT1_TYPE"), 0.0F);
}

void HeartbeatHandler::processMessage(const mavlink_message_t& message)
{
    const std::uint8_t expected = static_cast<std::uint8_t>(lastSequence_ + 1U);
    if (message.seq != expected) {
        qDebug() << "mavlink_seq prev:" << lastSequence_
                 << " cur:" << message.seq;
    }
    lastSequence_ = message.seq;

    if (message.msgid == MAVLINK_MSG_ID_AUTOPILOT_VERSION) {
        mavlink_autopilot_version_t version{};
        mavlink_msg_autopilot_version_decode(&message, &version);
        ardupilotVersion_ = fixedString(version.flight_custom_version, 8);
        qDebug() << "MAVLINK_MSG_ID_AUTOPILOT_VERSION:"
                 << " capabilities:" << version.capabilities
                 << " UID:" << version.uid
                 << " flight_sw_version:" << version.flight_sw_version
                 << " middleware_sw_version:"
                 << version.middleware_sw_version
                 << " os_sw_version:" << version.os_sw_version
                 << " board_version:" << version.board_version
                 << " vendor_id:" << version.vendor_id
                 << " product_id:" << version.product_id
                 << " flight_custom_version:" << ardupilotVersion_;
        if (ardupilotVersion_.compare(QStringLiteral("45INAV06"),
                                      Qt::CaseInsensitive)
                == 0
            || ardupilotVersion_.compare(QStringLiteral("45INAV07"),
                                         Qt::CaseInsensitive)
                == 0) {
            emit isValidArdupilotVersion();
        }
        return;
    }

    switch (message.msgid) {
    case MAVLINK_MSG_ID_HEARTBEAT: {
        mavlink_heartbeat_t heartbeat{};
        mavlink_msg_heartbeat_decode(&message, &heartbeat);
        if (supportedVehicleType(heartbeat.type)
            && currentMode_ != static_cast<int>(heartbeat.custom_mode)) {
            currentMode_ = static_cast<int>(heartbeat.custom_mode);
            emit currentModeChanged(currentMode_);
        }
        break;
    }
    case MAVLINK_MSG_ID_SYS_STATUS: {
        mavlink_sys_status_t status{};
        mavlink_msg_sys_status_decode(&message, &status);
        const bool armed =
            (status.onboard_control_sensors_health
             & MAV_SYS_STATUS_PREARM_CHECK)
            != 0U;
        if (!armedInitialized_ || armed_ != armed) {
            armed_ = armed;
            armedInitialized_ = true;
            qDebug() << (armed_ ? "armed sys status"
                                : "disarmed sys status");
        }

        bool compass = false;
        if ((status.onboard_control_sensors_present
             & MAV_SYS_STATUS_SENSOR_3D_MAG)
            != 0U) {
            compass =
                (status.onboard_control_sensors_enabled
                 & MAV_SYS_STATUS_SENSOR_3D_MAG)
                    != 0U
                && (status.onboard_control_sensors_health
                    & MAV_SYS_STATUS_SENSOR_3D_MAG)
                    != 0U;
        }
        if (compassEnabled_ != compass) {
            compassEnabled_ = compass;
            qDebug() << "compass status changed:" << compassEnabled_;
            emit compassStatusChanged(compassEnabled_);
        }
        qDebug() << "BATTARY remaining:" << status.battery_remaining
                 << " voltage:" << status.voltage_battery
                 << " current:" << status.current_battery;
        break;
    }
    case MAVLINK_MSG_ID_STATUSTEXT: {
        mavlink_statustext_t status{};
        mavlink_msg_statustext_decode(&message, &status);
        const QString text = fixedString(status.text, 50);
        qDebug() << "Mavlink status:" << text
                 << " status.chunk_seq:" << status.chunk_seq
                 << " status.id:" << status.id
                 << " status.severity:" << status.severity
                 << " sysid:" << message.sysid
                 << " compid:" << message.compid;
        if (text.contains(QStringLiteral("Homing On"))) {
            emit homingOn();
            return;
        }
        if (text.contains(QStringLiteral("Homing Off"))) {
            emit homingOff();
            return;
        }
        if (text.contains(QStringLiteral("Throttle armed"))) {
            armed_ = true;
            qDebug() << "armed by status text 'Throttle armed'";
            emit armed();
            return;
        }
        if (text.contains(QStringLiteral("Throttle disarmed"))) {
            armed_ = false;
            qDebug() << "disarmed by status text 'Throttle disarmed'";
            emit disarmed();
            return;
        }
        if (text.contains(QStringLiteral("LNCH_"))) {
            qDebug() << "LUA LAUNCH MSG:" << text << " id:" << status.id;
            if (readAcceleration_ && !accelerationEventSent_
                && text.contains(QStringLiteral("LNCH_DETE"))) {
                qDebug() << "acceleration 5G by LUA";
                QTimer::singleShot(static_cast<int>(accelerationDelayMs_),
                                   this,
                                   &HeartbeatHandler::launchAcceleration);
                setInterval(MAVLINK_MSG_ID_RAW_IMU, 1000000.0F);
                accelerationEventSent_ = true;
            }
        }
        break;
    }
    case MAVLINK_MSG_ID_MISSION_CURRENT: {
        mavlink_mission_current_t mission{};
        mavlink_msg_mission_current_decode(&message, &mission);
        if (currentMissionItem_ != mission.seq) {
            qDebug() << "current mission item idx:" << mission.seq;
            currentMissionItem_ = mission.seq;
            emit currentMissionItemChanged(currentMissionItem_);
        }
        break;
    }
    case MAVLINK_MSG_ID_RAW_IMU: {
        mavlink_raw_imu_t raw{};
        mavlink_msg_raw_imu_decode(&message, &raw);
        const std::uint32_t x = static_cast<std::uint32_t>(
            static_cast<std::int32_t>(raw.xacc)
            * static_cast<std::int32_t>(raw.xacc));
        const std::uint32_t y = static_cast<std::uint32_t>(
            static_cast<std::int32_t>(raw.yacc)
            * static_cast<std::int32_t>(raw.yacc));
        const std::uint32_t z = static_cast<std::uint32_t>(
            static_cast<std::int32_t>(raw.zacc)
            * static_cast<std::int32_t>(raw.zacc));
        lastAccelerationSquared_ =
            static_cast<std::int32_t>(x + y + z);
        if (accelerationLog_) {
            qDebug() << "acceleration::" << lastAcceleration();
        }
        if (readAcceleration_ && !accelerationEventSent_
            && lastAccelerationSquared_ > accelerationThresholdSquared_) {
            qDebug() << "acceleration 5G:" << lastAcceleration();
            QTimer::singleShot(static_cast<int>(accelerationDelayMs_),
                               this,
                               &HeartbeatHandler::launchAcceleration);
            setInterval(MAVLINK_MSG_ID_RAW_IMU, 1000000.0F);
            accelerationEventSent_ = true;
        }
        qDebug() << "RAW_IMU time:" << raw.time_usec
                 << " xacc:" << raw.xacc << " yacc:" << raw.yacc
                 << " zacc:" << raw.zacc << " xgyro:" << raw.xgyro
                 << " ygyro:" << raw.ygyro << " zgyro:" << raw.zgyro
                 << " xmag:" << raw.xmag << " ymag:" << raw.ymag
                 << " zmag:" << raw.zmag << " id:" << raw.id
                 << " temperature:" << raw.temperature;
        break;
    }
    case MAVLINK_MSG_ID_SCALED_IMU: {
        mavlink_scaled_imu_t scaled{};
        mavlink_msg_scaled_imu_decode(&message, &scaled);
        qDebug() << "SCALED_IMU time:" << scaled.time_boot_ms
                 << " xacc:" << scaled.xacc << " yacc:" << scaled.yacc
                 << " zacc:" << scaled.zacc << " xgyro:" << scaled.xgyro
                 << " ygyro:" << scaled.ygyro << " zgyro:" << scaled.zgyro
                 << " xmag:" << scaled.xmag << " ymag:" << scaled.ymag
                 << " zmag:" << scaled.zmag
                 << " temperature:" << scaled.temperature;
        break;
    }
    case MAVLINK_MSG_ID_VFR_HUD: {
        mavlink_vfr_hud_t hud{};
        mavlink_msg_vfr_hud_decode(&message, &hud);
        qDebug() << "VFR_HUD airspeed:" << hud.airspeed
                 << " groundspeed:" << hud.groundspeed << " alt:" << hud.alt
                 << " heading:" << hud.heading << " climb:" << hud.climb
                 << " throttle:" << hud.throttle;
        emit airspeedEvent(hud.airspeed, hud.groundspeed);
        break;
    }
    case MAVLINK_MSG_ID_PARAM_VALUE: {
        mavlink_param_value_t parameter{};
        mavlink_msg_param_value_decode(&message, &parameter);
        const QString name = fixedString(parameter.param_id, 16).left(16);
        qDebug() << "mavlink param:" << name
                 << " value:" << parameter.param_value;
        emit mavlinkParamValue(name, parameter.param_value);
        if (pendingParameters_.contains(name)) {
            qDebug() << "remove wait param:" << name;
            pendingParameters_.remove(name);
        }
        const auto written = writtenParameters_.constFind(name);
        if (written != writtenParameters_.cend()
            && written.value() != parameter.param_value) {
            qDebug() << "param '" << name << "' value:"
                     << parameter.param_value << " and last writed value:"
                     << written.value() << " not equals";
            writeParam(name, written.value());
        }
        break;
    }
    case MAVLINK_MSG_ID_SCALED_PRESSURE: {
        mavlink_scaled_pressure_t pressure{};
        mavlink_msg_scaled_pressure_decode(&message, &pressure);
        qDebug() << "SCALED_PRESSURE time:" << pressure.time_boot_ms
                 << " press_abs:" << pressure.press_abs
                 << " press_diff:" << pressure.press_diff
                 << " temperatue:" << pressure.temperature
                 << " temperature_press_diff:"
                 << pressure.temperature_press_diff;
        break;
    }
    case MAVLINK_MSG_ID_SCALED_PRESSURE2: {
        mavlink_scaled_pressure2_t pressure{};
        mavlink_msg_scaled_pressure2_decode(&message, &pressure);
        qDebug() << "SCALED_PRESSURE2 time:" << pressure.time_boot_ms
                 << " press_abs:" << pressure.press_abs
                 << " press_diff:" << pressure.press_diff
                 << " temperatue:" << pressure.temperature
                 << " temperature_press_diff:"
                 << pressure.temperature_press_diff;
        break;
    }
    case MAVLINK_MSG_ID_DISTANCE_SENSOR: {
        mavlink_distance_sensor_t distance{};
        mavlink_msg_distance_sensor_decode(&message, &distance);
        qDebug() << "DISTANCE_SENSOR time:" << distance.time_boot_ms
                 << " min_distance:" << distance.min_distance
                 << " max_distance:" << distance.max_distance
                 << " current_distance:" << distance.current_distance
                 << " type:" << distance.type << " id:" << distance.id
                 << " orientation:" << distance.orientation
                 << " covariance:" << distance.covariance
                 << " horizontal_fov:" << distance.horizontal_fov
                 << " vertical_fov:" << distance.vertical_fov
                 << " signal_quality:" << distance.signal_quality;
        emit distanceSensorEvent(20U, 17500U, distance.current_distance);
        break;
    }
    case MAVLINK_MSG_ID_OPTICAL_FLOW: {
        mavlink_optical_flow_t flow{};
        mavlink_msg_optical_flow_decode(&message, &flow);
        lastFlowX_ = static_cast<std::uint16_t>(flow.flow_x);
        lastFlowY_ = static_cast<std::uint16_t>(flow.flow_y);
        qDebug() << "OPTICAL_FLOW time:" << flow.time_usec
                 << " flow_comp_m_x:" << flow.flow_comp_m_x
                 << " flow_comp_m_y:" << flow.flow_comp_m_y
                 << " ground_distance:" << flow.ground_distance
                 << " flow_x:" << flow.flow_x << " flow_y:" << flow.flow_y
                 << " sensor_id:" << flow.sensor_id
                 << " quality:" << flow.quality
                 << " flow_rate_x:" << flow.flow_rate_x
                 << " flow_rate_y:" << flow.flow_rate_y;
        break;
    }
    case MAVLINK_MSG_ID_GIMBAL_DEVICE_ATTITUDE_STATUS: {
        mavlink_gimbal_device_attitude_status_t status{};
        mavlink_msg_gimbal_device_attitude_status_decode(&message, &status);
        qDebug() << "GIMBAL_DEVICE_ATTITUDE_STATUS time:"
                 << status.time_boot_ms << " compId:" << message.compid
                 << " qw:" << status.q[0] << " qx:" << status.q[1]
                 << " qy:" << status.q[2] << " qz:" << status.q[3]
                 << " angular_velocity_x:" << status.angular_velocity_x
                 << " angular_velocity_y:" << status.angular_velocity_y
                 << " angular_velocity_z:" << status.angular_velocity_z
                 << " delta_yaw:" << status.delta_yaw
                 << " delta_yaw_velocity:" << status.delta_yaw_velocity
                 << " gimbal_device_id:" << status.gimbal_device_id;
        if (qIsNaN(status.angular_velocity_x)
            || qIsNaN(status.angular_velocity_y)
            || qIsNaN(status.angular_velocity_z)) {
            qDebug() << "ignore GIMBAL_DEVICE_ATTITUDE_STATUS";
            break;
        }
        emit gimbalDeviceAttitudeStatus(
            static_cast<qlonglong>(status.time_boot_ms),
            message.compid,
            status.gimbal_device_id,
            status.q[0],
            status.q[1],
            status.q[2],
            status.q[3],
            status.angular_velocity_x,
            status.angular_velocity_y,
            status.angular_velocity_z,
            status.delta_yaw,
            status.delta_yaw_velocity);
        break;
    }
    default:
        break;
    }
}
