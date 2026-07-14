#include "camera/camera.hpp"

#include "camera/gimbal_transform.hpp"

#include <QDateTime>

#include <algorithm>
#include <cmath>

namespace {

float normalize360(float value)
{
    value = std::fmod(value, 360.0F);
    return value < 0.0F ? value + 360.0F : value;
}

float angularDelta(float from, float to)
{
    float value = normalize360(to) - normalize360(from);
    if (value > 180.0F) {
        value -= 360.0F;
    } else if (value < -180.0F) {
        value += 360.0F;
    }
    return value;
}

template <typename T>
const T* historyItem(const QVector<T>& values,
                     int currentIndex,
                     qlonglong time,
                     int* nextItem = nullptr)
{
    if (currentIndex < 0 || values.isEmpty()) {
        return nullptr;
    }
    if (time == 0 || values[currentIndex].time <= time) {
        if (nextItem) {
            *nextItem = currentIndex;
        }
        return &values[currentIndex];
    }
    int index = currentIndex;
    for (int count = 0; count < values.size(); ++count) {
        const int previous = (index - 1 + values.size()) % values.size();
        if (values[previous].time == 0) {
            break;
        }
        if (values[previous].time <= time) {
            if (nextItem) {
                *nextItem = index;
            }
            return &values[previous];
        }
        index = previous;
    }
    if (nextItem) {
        *nextItem = index;
    }
    return &values[index];
}

} // namespace

Camera::Camera(QObject* parent)
    : QObject(parent)
    , gimbalHistory_(kHistorySize)
    , zoomHistory_(kHistorySize)
{
    qRegisterMetaType<Gimbal>("Gimbal");
    qRegisterMetaType<CameraParams>("CameraParams");
}

bool Camera::deviceIsEnabled() const { return deviceEnabled_; }
bool Camera::isConnected() const { return connected_; }
bool Camera::isGimbalStabRoll() const { return gimbalStabilizeRoll_; }

float Camera::calibrationWidthAngle() const
{
    return calibrationZooms_.isEmpty() ? 0.0F
                                       : calibrationZooms_.constLast().widthAngle;
}

float Camera::calibrationHeightAngle() const
{
    return calibrationZooms_.isEmpty()
        ? 0.0F
        : calibrationZooms_.constLast().heightAngle;
}

CameraParams Camera::currentParams() const
{
    CameraParams value;
    value.zoomControlEnabled = zoomControlEnabled_;
    value.externalGimbalControl = externalGimbalControl_;
    value.gimbal = currentGimbal_;
    value.reserved60 = reservedA_;
    value.reserved64 = reservedB_;
    value.reserved68 = reservedC_;
    value.frameWidth = frameWidth_;
    value.frameHeight = frameHeight_;
    value.zoom = currentZoom_;
    value.calibrations = calibrationZooms_;
    return value;
}

Gimbal Camera::gimbal(qlonglong time) const
{
    if (time == 0 || currentGimbal_.time <= time || gimbalIndex_ < 0) {
        return currentGimbal_;
    }
    int next = -1;
    const Gimbal* const before = historyItem(gimbalHistory_, gimbalIndex_,
                                             time, &next);
    if (!before || next < 0 || next == gimbalIndex_ ||
        gimbalHistory_[next].time <= before->time) {
        return before ? *before : currentGimbal_;
    }
    const Gimbal& after = gimbalHistory_[next];
    const float alpha = static_cast<float>(time - before->time) /
        static_cast<float>(after.time - before->time);
    Gimbal value = *before;
    const auto lerp = [alpha](float a, float b) {
        return a + (b - a) * alpha;
    };
    value.roll = lerp(before->roll, after.roll);
    value.pitch = lerp(before->pitch, after.pitch);
    value.yaw = normalize360(before->yaw + angularDelta(before->yaw,
                                                        after.yaw) * alpha);
    value.rollMaxSpeed = lerp(before->rollMaxSpeed, after.rollMaxSpeed);
    value.pitchMaxSpeed = lerp(before->pitchMaxSpeed, after.pitchMaxSpeed);
    value.yawMaxSpeed = lerp(before->yawMaxSpeed, after.yawMaxSpeed);
    value.magRoll = lerp(before->magRoll, after.magRoll);
    value.magPitch = lerp(before->magPitch, after.magPitch);
    value.magYaw = normalize360(before->magYaw + angularDelta(before->magYaw,
                                                              after.magYaw) * alpha);
    value.vehicleRoll = lerp(before->vehicleRoll, after.vehicleRoll);
    value.vehiclePitch = lerp(before->vehiclePitch, after.vehiclePitch);
    value.vehicleYaw = normalize360(before->vehicleYaw +
                                    angularDelta(before->vehicleYaw,
                                                 after.vehicleYaw) * alpha);
    value.time = time;
    return value;
}

Zoom Camera::zoom(qlonglong time) const
{
    if (time == 0 || currentZoom_.time <= time || zoomIndex_ < 0) {
        return currentZoom_;
    }
    int next = -1;
    const Zoom* const before = historyItem(zoomHistory_, zoomIndex_, time,
                                           &next);
    if (!before || next < 0 || next == zoomIndex_ ||
        zoomHistory_[next].time <= before->time) {
        return before ? *before : currentZoom_;
    }
    const Zoom& after = zoomHistory_[next];
    const float alpha = static_cast<float>(time - before->time) /
        static_cast<float>(after.time - before->time);
    const auto lerp = [alpha](float a, float b) {
        return a + (b - a) * alpha;
    };
    Zoom value;
    value.opticalZoom = lerp(before->opticalZoom, after.opticalZoom);
    value.percent = lerp(before->percent, after.percent);
    value.widthAngle = lerp(before->widthAngle, after.widthAngle);
    value.heightAngle = lerp(before->heightAngle, after.heightAngle);
    value.scaleX = lerp(before->scaleX, after.scaleX);
    value.scaleY = lerp(before->scaleY, after.scaleY);
    value.time = time;
    return value;
}

void Camera::setGimbal(const Gimbal& value)
{
    if (!externalGimbalControl_) {
        return;
    }
    const float roll = std::clamp(value.roll, -50.0F, 50.0F);
    const float pitch = std::clamp(value.pitch, -130.0F, 65.0F);
    if (currentGimbal_.roll != roll || currentGimbal_.pitch != pitch ||
        currentGimbal_.yaw != value.yaw ||
        currentGimbal_.center != value.center) {
        emit setPTZ(roll,
                    pitch,
                    value.yaw,
                    value.rollMaxSpeed,
                    value.pitchMaxSpeed,
                    value.yawMaxSpeed,
                    value.center,
                    value.yawByNorth,
                    gimbalId_);
    }
}

void Camera::setGimbalStabRoll(bool enabled)
{
    gimbalStabilizeRoll_ = enabled;
}

void Camera::setFrameBlur(float value) { frameBlur_ = value; }

void Camera::setControlFeatures(bool zoomControlEnabled,
                                bool externalGimbalControl)
{
    zoomControlEnabled_ = zoomControlEnabled;
    externalGimbalControl_ = externalGimbalControl;
}

void Camera::setDeviceEnabledState(bool enabled)
{
    if (deviceEnabled_ == enabled) {
        return;
    }
    deviceEnabled_ = enabled;
    if (enabled) {
        emit deviceEnabled();
    } else {
        emit deviceDisabled();
    }
}

void Camera::setConnectedState(bool connectedState)
{
    if (connected_ == connectedState) {
        return;
    }
    connected_ = connectedState;
    if (connectedState) {
        emit connected();
    } else {
        emit disconnected();
    }
}

QVector<Zoom>& Camera::calibrationZooms() { return calibrationZooms_; }
const QVector<Zoom>& Camera::calibrationZooms() const
{
    return calibrationZooms_;
}

void Camera::saveCurrentGimbal()
{
    if (gimbalIndex_ >= 0 &&
        currentGimbal_.time <= gimbalHistory_[gimbalIndex_].time) {
        gimbalHistory_[gimbalIndex_] = currentGimbal_;
        return;
    }
    gimbalIndex_ = (gimbalIndex_ + 1) % kHistorySize;
    gimbalHistory_[gimbalIndex_] = currentGimbal_;
    gimbalHistoryInitialized_ = gimbalHistoryInitialized_ || gimbalIndex_ == 1;
}

void Camera::saveCurrentZoom()
{
    zoomIndex_ = (zoomIndex_ + 1) % kHistorySize;
    zoomHistory_[zoomIndex_] = currentZoom_;
    zoomHistoryInitialized_ = zoomHistoryInitialized_ || zoomIndex_ == 1;
}

void Camera::gimbalValues(float roll,
                          float pitch,
                          float yaw,
                          bool isCenter,
                          qlonglong time)
{
    if (currentGimbal_.roll == roll && currentGimbal_.pitch == pitch &&
        currentGimbal_.yaw == yaw && currentGimbal_.center == isCenter) {
        return;
    }
    currentGimbal_.time = time;
    currentGimbal_.roll = roll;
    currentGimbal_.pitch = pitch;
    currentGimbal_.yaw = yaw;
    currentGimbal_.center = isCenter;
    currentGimbal_.deltaYaw = 0.0F;
    currentGimbal_.deltaYawVelocity = 0.0F;
    saveCurrentGimbal();
    emit paramsChanged(currentParams());
}

void Camera::calculateAnglesFromMag(Gimbal& value) const
{
    const RPY result = body2camera(
        {value.vehicleRoll, value.vehiclePitch, value.vehicleYaw},
        {value.magRoll, value.magPitch, value.magYaw});
    value.roll = result.roll;
    value.pitch = result.pitch;
    value.yaw = normalize360(result.yaw - value.vehicleYaw);
    if (value.yaw > 180.0F) {
        value.yaw -= 360.0F;
    }
}

void Camera::gimbalMagValues(qlonglong time,
                             unsigned char componentId,
                             float roll,
                             float pitch,
                             float yaw,
                             float magRoll,
                             float magPitch,
                             float magYaw)
{
    currentGimbal_.time = time;
    currentGimbal_.hasMagRoll = true;
    currentGimbal_.hasMagPitch = true;
    currentGimbal_.hasMagYaw = true;
    currentGimbal_.gimbalId = componentId;
    currentGimbal_.vehicleRoll = roll;
    currentGimbal_.vehiclePitch = pitch;
    currentGimbal_.vehicleYaw = yaw;
    currentGimbal_.magRoll = magRoll;
    currentGimbal_.magPitch = magPitch;
    currentGimbal_.magYaw = magYaw;
    calculateAnglesFromMag(currentGimbal_);
    saveCurrentGimbal();
}

void Camera::gimbalRollMagValue(qlonglong time,
                                unsigned char componentId,
                                float roll,
                                float pitch,
                                float yaw,
                                float magRoll)
{
    currentGimbal_.magPitch = 0.0F;
    currentGimbal_.magYaw = 0.0F;
    gimbalMagValues(time, componentId, roll, pitch, yaw, magRoll, 0.0F, 0.0F);
    currentGimbal_.hasMagPitch = false;
    currentGimbal_.hasMagYaw = false;
}

void Camera::gimbalPitchMagValue(qlonglong time,
                                 unsigned char componentId,
                                 float roll,
                                 float pitch,
                                 float yaw,
                                 float magPitch)
{
    gimbalMagValues(time, componentId, roll, pitch, yaw, 0.0F, magPitch, 0.0F);
    currentGimbal_.hasMagRoll = false;
    currentGimbal_.hasMagYaw = false;
}

void Camera::gimbalYawMagValue(qlonglong time,
                               unsigned char componentId,
                               float roll,
                               float pitch,
                               float yaw,
                               float magYaw)
{
    gimbalMagValues(time, componentId, roll, pitch, yaw, 0.0F, 0.0F, magYaw);
    currentGimbal_.hasMagRoll = false;
    currentGimbal_.hasMagPitch = false;
}

void Camera::setGimbalAHRS(qlonglong time,
                           float gimbalRoll,
                           float gimbalPitch,
                           float gimbalYaw)
{
    const Gimbal value = gimbal(time);
    const RPY body = camera2body({gimbalRoll, gimbalPitch, gimbalYaw},
                                 {value.roll, value.pitch, value.yaw});
    emit ahrsByCamera(time, body.roll, body.pitch, body.yaw);
}

void Camera::updateZoom(float percent, qlonglong time)
{
    percent = std::clamp(percent, 0.0F, 100.0F);
    currentZoom_.percent = percent;
    currentZoom_.time = time == 0 ? QDateTime::currentMSecsSinceEpoch() : time;
    if (!calibrationZooms_.isEmpty()) {
        const Zoom* lower = &calibrationZooms_.first();
        const Zoom* upper = lower;
        for (const Zoom& item : calibrationZooms_) {
            upper = &item;
            if (item.percent >= percent) {
                break;
            }
            lower = &item;
        }
        const float span = upper->percent - lower->percent;
        const float alpha = span == 0.0F ? 0.0F
                                         : (percent - lower->percent) / span;
        const auto lerp = [alpha](float a, float b) {
            return a + (b - a) * alpha;
        };
        currentZoom_.opticalZoom =
            lerp(lower->opticalZoom, upper->opticalZoom);
        currentZoom_.widthAngle = lerp(lower->widthAngle, upper->widthAngle);
        currentZoom_.heightAngle =
            lerp(lower->heightAngle, upper->heightAngle);
        currentZoom_.scaleX = lerp(lower->scaleX, upper->scaleX);
        currentZoom_.scaleY = lerp(lower->scaleY, upper->scaleY);
    }
    saveCurrentZoom();
    emit paramsChanged(currentParams());
}

void Camera::applyZoomRecord(const Zoom& value, qlonglong time)
{
    currentZoom_ = value;
    currentZoom_.time = time == 0 ? QDateTime::currentMSecsSinceEpoch() : time;
    saveCurrentZoom();
    emit paramsChanged(currentParams());
}

void Camera::updateZoomFromOptical(float optical, qlonglong time)
{
    if (calibrationZooms_.isEmpty()) {
        Zoom value = currentZoom_;
        value.opticalZoom = optical;
        applyZoomRecord(value, time);
        return;
    }

    const Zoom* lower = &calibrationZooms_.first();
    const Zoom* upper = lower;
    for (const Zoom& item : calibrationZooms_) {
        upper = &item;
        if (item.opticalZoom >= optical) {
            break;
        }
        lower = &item;
    }
    const float span = upper->opticalZoom - lower->opticalZoom;
    const float alpha = span == 0.0F ? 0.0F
                                     : (optical - lower->opticalZoom) / span;
    const auto lerp = [alpha](float a, float b) {
        return a + (b - a) * alpha;
    };
    Zoom value;
    value.opticalZoom = optical;
    value.percent = lerp(lower->percent, upper->percent);
    value.widthAngle = lerp(lower->widthAngle, upper->widthAngle);
    value.heightAngle = lerp(lower->heightAngle, upper->heightAngle);
    value.scaleX = lerp(lower->scaleX, upper->scaleX);
    value.scaleY = lerp(lower->scaleY, upper->scaleY);
    applyZoomRecord(value, time);
}

void Camera::updateGimbalTelemetry(float roll,
                                   float pitch,
                                   float yaw,
                                   float rollVelocity,
                                   float pitchVelocity,
                                   float yawVelocity,
                                   qlonglong time)
{
    currentGimbal_.time = time;
    currentGimbal_.roll = roll;
    currentGimbal_.pitch = pitch;
    currentGimbal_.yaw = yaw;
    currentGimbal_.angularVelocityRoll = rollVelocity;
    currentGimbal_.angularVelocityPitch = pitchVelocity;
    currentGimbal_.angularVelocityYaw = yawVelocity;
    saveCurrentGimbal();
    emit paramsChanged(currentParams());
}

int Camera::previousIndex(int index) const
{
    return index <= 0 ? kHistorySize - 1 : index - 1;
}
int Camera::nextIndex(int index) const
{
    return index + 1 >= kHistorySize ? 0 : index + 1;
}
