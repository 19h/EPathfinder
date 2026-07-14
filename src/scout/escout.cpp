#include "scout/escout.hpp"

#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QTimerEvent>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

namespace {

constexpr double kCoordinateScale = 1.0e-7;
constexpr double kMillimetresPerMetre = 1000.0;
constexpr float kRadiansToDegrees = 57.296F;

qlonglong nowMs()
{
    return QDateTime::currentMSecsSinceEpoch();
}

float clampFinite(float value, float low, float high, float fallback = 0.0F)
{
    if (!std::isfinite(value)) {
        return fallback;
    }
    return std::clamp(value, low, high);
}

QGeoCoordinate coordinateOf(const ECoord& value)
{
    if (!value.valid) {
        return {};
    }
    return QGeoCoordinate(static_cast<double>(value.lat) * kCoordinateScale,
                          static_cast<double>(value.lon) * kCoordinateScale,
                          static_cast<double>(value.alt) /
                              kMillimetresPerMetre);
}

ECoord coordPrefix(const EPosition& value)
{
    return static_cast<const ECoord&>(value);
}

} // namespace

static_assert(sizeof(ECoord) == 0x20, "ECoord ABI reconstruction drifted");
static_assert(sizeof(EPosition) == 0x58,
              "EPosition ABI reconstruction drifted");
static_assert(sizeof(EAttitude) == 0x30,
              "EAttitude ABI reconstruction drifted");
static_assert(sizeof(ERawGpsPosition) == 0x28,
              "ERawGpsPosition ABI reconstruction drifted");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
static_assert(sizeof(RoadValidateResult) == 0x58,
              "Qt 5 RoadValidateResult ABI reconstruction drifted");
#endif

EScout::EScout(QObject* parent, bool isCopter)
    : QObject(parent)
    , positions_(kHistorySize)
    , attitudes_(kHistorySize)
    , rawGpsPositions_(kHistorySize)
    , rawImuPositions_(kHistorySize)
    , imuPositions_(kHistorySize)
    , ePositions_(kHistorySize)
    , eBackupPositions_(kHistorySize)
    , eAttitudes_(kHistorySize)
    , isCopter_(isCopter)
{
    qRegisterMetaType<RoadValidateResult>("RoadValidateResult");
    qRegisterMetaType<IMUCorrectSource>("IMUCorrectSource");
    qRegisterMetaType<GPSUsageMode>("GPSUsageMode");
    qRegisterMetaType<DistanceSensorUsageMode>("DistanceSensorUsageMode");
    startTimer(500);
    readExactMagTable();
}

template <typename T>
int EScout::appendRing(QVector<T>& values,
                       int& currentIndex,
                       bool& initialized)
{
    if (values.isEmpty()) {
        values.resize(kHistorySize);
    }
    currentIndex = (currentIndex + 1) % values.size();
    initialized = true;
    values[currentIndex] = T{};
    return currentIndex;
}

template <typename T>
int EScout::findItem(const QVector<T>& values,
                     int currentIndex,
                     qlonglong time) const
{
    if (currentIndex < 0 || currentIndex >= values.size()) {
        return -1;
    }
    if (time <= 0 || values[currentIndex].time <= time) {
        return currentIndex;
    }

    int index = currentIndex;
    for (int scanned = 0; scanned < values.size(); ++scanned) {
        const int previous = (index - 1 + values.size()) % values.size();
        if (values[previous].time == 0) {
            return index;
        }
        if (values[previous].time <= time) {
            return previous;
        }
        index = previous;
    }
    return index;
}

float EScout::normalize360(float angle)
{
    if (!std::isfinite(angle)) {
        return 0.0F;
    }
    angle = std::fmod(angle, 360.0F);
    return angle < 0.0F ? angle + 360.0F : angle;
}

float EScout::signedAngularDelta(float from, float to)
{
    float result = normalize360(to) - normalize360(from);
    if (result > 180.0F) {
        result -= 360.0F;
    } else if (result < -180.0F) {
        result += 360.0F;
    }
    return result;
}

EPosition EScout::projectedPosition(const QVector<EPosition>& values,
                                    int currentIndex,
                                    qulonglong requestedTime,
                                    bool eLink) const
{
    if (currentIndex < 0) {
        return {};
    }
    const qlonglong targetTime = requestedTime == 0
        ? nowMs()
        : static_cast<qlonglong>(requestedTime);
    const int index = findItem(values, currentIndex, targetTime);
    if (index < 0) {
        return {};
    }

    EPosition result = values[index];
    const qlonglong elapsedMs = std::max<qlonglong>(0, targetTime - result.time);
    QGeoCoordinate coordinate = coordinateOf(result);
    if (coordinate.isValid() && elapsedMs > 0) {
        const double northMetres =
            static_cast<double>(result.vx) * elapsedMs / 100000.0;
        const double eastMetres =
            static_cast<double>(result.vy) * elapsedMs / 100000.0;
        coordinate = coordinate.atDistanceAndAzimuth(northMetres, 0.0);
        coordinate = coordinate.atDistanceAndAzimuth(eastMetres, 90.0);
        coordinate.setAltitude(
            coordinate.altitude() -
            static_cast<double>(result.vz) * elapsedMs / 100000.0);
        result.lat = qRound64(coordinate.latitude() / kCoordinateScale);
        result.lon = qRound64(coordinate.longitude() / kCoordinateScale);
        result.alt = qRound64(coordinate.altitude() * kMillimetresPerMetre);
        result.mslAlt -= static_cast<int>(
            static_cast<qlonglong>(result.vz) * elapsedMs / 100);
    }

    if (eLink) {
        if (distanceSensorOffset_ != 0) {
            result.alt += distanceSensorOffset_;
        } else if (currentGroundMsl_ != 0) {
            result.alt = result.mslAlt - currentGroundMsl_;
        }
    } else {
        result.alt += targetAltitudeOffset_ + distanceSensorOffset_;
    }
    result.time = targetTime;
    result.secondaryType = 0;
    result.secondaryLat = 0;
    result.secondaryLon = 0;
    result.secondaryAlt = 0;
    result.secondaryMslAlt = 0;
    result.secondaryValid = false;
    return result;
}

EAttitude EScout::projectedAttitude(const QVector<EAttitude>& values,
                                    int currentIndex,
                                    qulonglong requestedTime,
                                    bool applyYawOffset,
                                    bool eLink)
{
    if (currentIndex < 0) {
        return {};
    }
    const qlonglong targetTime = requestedTime == 0
        ? nowMs()
        : static_cast<qlonglong>(requestedTime);
    const int index = findItem(values, currentIndex, targetTime);
    if (index < 0) {
        return {};
    }

    EAttitude result = values[index];
    const int next = (index + 1) % values.size();
    if (index != currentIndex && values[next].time > result.time &&
        values[next].time >= targetTime) {
        const float span = static_cast<float>(values[next].time - result.time);
        const float alpha = span > 0.0F
            ? static_cast<float>(targetTime - result.time) / span
            : 0.0F;
        result.pitch += (values[next].pitch - result.pitch) * alpha;
        result.roll += (values[next].roll - result.roll) * alpha;
        result.yaw = normalize360(
            result.yaw + signedAngularDelta(result.yaw, values[next].yaw) * alpha);
        result.pitchSpeed +=
            (values[next].pitchSpeed - result.pitchSpeed) * alpha;
        result.rollSpeed +=
            (values[next].rollSpeed - result.rollSpeed) * alpha;
        result.yawSpeed += (values[next].yawSpeed - result.yawSpeed) * alpha;
    } else {
        const qlonglong elapsedMs = std::max<qlonglong>(0,
                                                        targetTime - result.time);
        if (elapsedMs >= 2) {
            result.pitch += result.pitchSpeed * elapsedMs;
            result.roll += result.rollSpeed * elapsedMs;
            result.yaw = normalize360(result.yaw + result.yawSpeed * elapsedMs);
        }
    }

    if (!eLink && rollPitchCalibrated_) {
        result.pitch += calibrationPitchOffset_;
        result.roll += calibrationRollOffset_;
    }
    if (!eLink && yawCalibrated_) {
        result.yaw = normalize360(result.yaw + calibrationYawOffset_);
    }
    if (applyYawOffset && !isCopter_) {
        result.yaw = normalize360(result.yaw + currentYawOffset());
    }
    if (replaceYawFromExactMag_ && exactMagValueValid_) {
        result.yaw = normalize360(static_cast<float>(exactMagValue_));
    }
    result.futurePitchSpeed = result.pitchSpeed;
    result.futureRollSpeed = result.rollSpeed;
    result.futureYaw = result.yaw;
    result.time = targetTime;
    return result;
}

EPosition EScout::position(qulonglong time)
{
    return projectedPosition(positions_, positionIndex_, time, false);
}

EPosition EScout::ePosition(qulonglong time)
{
    return projectedPosition(ePositions_, ePositionIndex_, time, true);
}

EAttitude EScout::attitude(qulonglong time, bool applyYawOffset)
{
    return projectedAttitude(attitudes_, attitudeIndex_, time, applyYawOffset,
                             false);
}

EAttitude EScout::eAttitude(qulonglong time)
{
    return projectedAttitude(eAttitudes_, eAttitudeIndex_, time, false, true);
}

EAttitude EScout::mainControllerAttitude(qulonglong time)
{
    return attitude(time, false);
}

ECoord EScout::ECoordOnLaunch() const { return launchCoord_; }
QGeoCoordinate EScout::QCoordOnLaunch() const { return launchQCoord_; }
int EScout::mslOnLaunch() const { return launchMsl_; }
int EScout::currentGroundMSL() const { return currentGroundMsl_; }
int EScout::distanceSensorOffset() const { return distanceSensorOffset_; }

int EScout::currentState() const
{
    if (!forceDisableGps_ && mainGpsState_ == 1) {
        return 1;
    }
    if (gpsUsageMode_ != GPS_NOT_USE && eLinkSatelliteStatus_ == 2) {
        return 2;
    }
    return 0;
}

qlonglong EScout::lastPositionTime() const
{
    return positionIndex_ < 0 ? 0 : positions_[positionIndex_].time;
}
qlonglong EScout::lastAttitudeTime() const
{
    return attitudeIndex_ < 0 ? 0 : attitudes_[attitudeIndex_].time;
}
qlonglong EScout::lastEPositionTime() const
{
    return ePositionIndex_ < 0 ? 0 : ePositions_[ePositionIndex_].time;
}
qlonglong EScout::lastEAttitudeTime() const
{
    return eAttitudeIndex_ < 0 ? 0 : eAttitudes_[eAttitudeIndex_].time;
}
qlonglong EScout::lastELinkEventTime() const { return lastELinkEventTime_; }
int EScout::satelliteCount() const { return satelliteCount_; }
int EScout::eLinkSatelliteCount() const { return eLinkSatelliteCount_; }
int EScout::eLinkSatelliteStatus() const { return eLinkSatelliteStatus_; }
int EScout::eLinkBackupSatelliteCount() const
{
    return eLinkBackupSatelliteCount_;
}
unsigned int EScout::distSensorValue() const { return distanceSensorValue_; }
qlonglong EScout::distSensorLastTime() const { return distanceSensorLastTime_; }
qlonglong EScout::lastRecalcPositionsTime() const
{
    return lastRecalcPositionsTime_;
}
GPSUsageMode EScout::currentGPSUsageMode() const { return gpsUsageMode_; }
bool EScout::roadIsEnabled() const { return roadsEnabled_; }
bool EScout::correctRoadDistByGPSIsEnabled() const
{
    return correctRoadDistanceByGpsEnabled_;
}
bool EScout::isPTZDistSensor() const { return ptzDistanceSensor_; }
bool EScout::lastRoadOffsetAccepted() const
{
    return hasRoadOffset_ && roadOffset_.accepted;
}
qlonglong EScout::lastRoadOffsetTime() const { return roadOffset_.time; }
float EScout::lastRoadOffsetSize() const { return roadOffset_.offset; }
qlonglong EScout::lastSavedCoordinateTime() const
{
    return lastSavedCoordinateTime_;
}

float EScout::groundSpeed() const
{
    if (positionIndex_ < 0) {
        return 0.0F;
    }
    const EPosition& value = positions_[positionIndex_];
    const float speed = std::hypot(static_cast<float>(value.vx),
                                   static_cast<float>(value.vy)) /
        100.0F;
    return std::clamp(speed, 3.0F, 60.0F);
}

float EScout::gpsGroundSpeed() const
{
    return positionIndex_ < 0 ? 0.0F : positions_[positionIndex_].groundSpeed;
}

qlonglong EScout::lastCalculatedWindTime() const
{
    return lastCalculatedWindTime_;
}
Wind EScout::lastCalculatedWind() const { return lastCalculatedWind_; }
Wind EScout::lastCalculatedAverageWind() const
{
    return lastCalculatedAverageWind_;
}

float EScout::currentAhrsRPErrorAngle() const
{
    return nowMs() - ahrsErrorTime_ <= 999 ? ahrsRpError_ : 0.0F;
}
float EScout::currentAhrsYawErrorAngle() const
{
    return nowMs() - ahrsErrorTime_ <= 999 ? ahrsYawError_ : 0.0F;
}
qlonglong EScout::lastVNavCorrectTime() const { return lastVNavCorrectTime_; }
float EScout::lastVNavCorrectDistance() const
{
    return lastVNavCorrectDistance_;
}
float EScout::lastVNavCorrectAzimuth() const
{
    return lastVNavCorrectAzimuth_;
}
qlonglong EScout::lastVNav2FixTime() const { return lastVNav2FixTime_; }
qlonglong EScout::lastVNav2TrackingTime() const
{
    return lastVNav2TrackingTime_;
}
bool EScout::isExactMagValue() const { return exactMagValueValid_; }
bool EScout::hasRoadOffsets() const { return hasRoadOffset_; }
bool EScout::odoIsConnected() const
{
    return odometrySocket_ != nullptr &&
        odometrySocket_->state() == QAbstractSocket::BoundState;
}

float EScout::azimuth() { return attitude().yaw; }
float EScout::gpsAzimuth()
{
    return positionIndex_ < 0 ? 0.0F : positions_[positionIndex_].azimuth;
}

float EScout::currentVnavWindSpeed()
{
    if (vnavWindTime_ == 0 || vnavWindSpeed_ == 0.0F) {
        return 0.0F;
    }
    const qlonglong age = std::max<qlonglong>(0, nowMs() - vnavWindTime_);
    const qlonglong halfLife = vnavWindLifetimeMs_ / 2;
    if (age <= halfLife) {
        return vnavWindSpeed_;
    }
    if (age >= vnavWindLifetimeMs_) {
        return 0.0F;
    }
    return vnavWindSpeed_ * static_cast<float>(vnavWindLifetimeMs_ - age) /
        static_cast<float>(std::max<qlonglong>(1, halfLife));
}

qlonglong EScout::mav2SysTime(qlonglong time) const
{
    return time + timeOffset_;
}
qlonglong EScout::sys2MavTime(qlonglong time) const
{
    return time - timeOffset_;
}

void EScout::saveLastCoord(double latitude, double longitude)
{
    launchQCoord_.setLatitude(latitude);
    launchQCoord_.setLongitude(longitude);
    launchCoord_.lat = qRound64(latitude / kCoordinateScale);
    launchCoord_.lon = qRound64(longitude / kCoordinateScale);
    launchCoord_.valid = launchQCoord_.isValid();
    lastSavedCoordinateTime_ = nowMs();
}

void EScout::setLaunchTime(qlonglong time) { launchTime_ = time; }
void EScout::setEDataEnabled(bool enabled) { eDataEnabled_ = enabled; }
void EScout::setRawGpsAltitudeEnabled(bool enabled)
{
    rawGpsAltitudeEnabled_ = enabled;
}

void EScout::setWind(const Wind& wind)
{
    wind_ = wind;
    lastCalculatedWind_ = wind;
    lastCalculatedWindTime_ = nowMs();
    realWinds_.append(wind);
    while (realWinds_.size() > 100) {
        realWinds_.removeFirst();
    }
    if (!realWinds_.isEmpty()) {
        double x = 0.0;
        double y = 0.0;
        double speed = 0.0;
        for (const Wind& item : realWinds_) {
            const double angle = qDegreesToRadians(
                static_cast<double>(item.direction));
            x += std::cos(angle);
            y += std::sin(angle);
            speed += item.speed;
        }
        lastCalculatedAverageWind_.direction = normalize360(
            static_cast<float>(qRadiansToDegrees(std::atan2(y, x))));
        lastCalculatedAverageWind_.speed =
            static_cast<float>(speed / realWinds_.size());
    }
}

void EScout::setEIMUAirspeed(float airspeed) { eImuAirspeed_ = airspeed; }
void EScout::setRoadRunner(RoadRunner* roadRunner) { roadRunner_ = roadRunner; }
void EScout::addTargetAltOffset(int offset) { targetAltitudeOffset_ += offset; }
void EScout::clearTargetAltOffset() { targetAltitudeOffset_ = 0; }
void EScout::setElevationsPath(const QString& path)
{
    elevationsPath_ = path;
}

void EScout::setOdometry(int port, EVision* vision)
{
    odometryPort_ = port;
    vision_ = vision;
    if (!odometrySocket_) {
        odometrySocket_ = new QUdpSocket(this);
        connect(odometrySocket_, &QUdpSocket::readyRead, this,
                &EScout::readOdoPendingDatagrams);
    }
    if (odometryEnabled_) {
        upConnectOdometry();
    }
}

void EScout::setLerpRollPitchSpeedCoeff(float coefficient)
{
    lerpRollPitchSpeedCoeff_ = clampFinite(coefficient, 0.0F, 1.0F, 0.8F);
}
void EScout::setForceDisableGPS(bool disable)
{
    forceDisableGps_ = disable;
    setSatelliteCount(satelliteCount_);
}

void EScout::setSatelliteCount(int count)
{
    const bool wasValid = satelliteCount_ > 3 && !forceDisableGps_;
    satelliteCount_ = std::max(0, count);
    mainGpsState_ = satelliteCount_ > 3 && !forceDisableGps_ ? 1 : 0;
    const bool isValidNow = mainGpsState_ == 1;
    if (wasValid != isValidNow) {
        if (isValidNow) {
            emit valid();
        } else {
            emit invalid();
        }
    }
}

void EScout::setDistanceSensorDivingEnabled(bool enabled)
{
    distanceSensorDivingEnabled_ = enabled;
}
void EScout::setDistanceSensorUsageMode(DistanceSensorUsageMode mode)
{
    distanceSensorUsageMode_ = mode;
}
void EScout::setIsDiving(bool diving, qlonglong time)
{
    isDiving_ = diving;
    divingTime_ = time;
}
void EScout::setGPSUsageMode(GPSUsageMode mode) { gpsUsageMode_ = mode; }
void EScout::setRoadsEnabled(bool enabled) { roadsEnabled_ = enabled; }
void EScout::setCorrectRoadDistByGPSEnabled(bool enabled)
{
    correctRoadDistanceByGpsEnabled_ = enabled;
}
void EScout::setPTZDistSensor(bool enabled) { ptzDistanceSensor_ = enabled; }
void EScout::setCorrectRoadByGPSMaxOffset(float offset)
{
    correctRoadByGpsMaxOffset_ = std::max(0.0F, offset);
}
void EScout::setIsCopter(bool isCopter) { isCopter_ = isCopter; }

void EScout::addMSL(double latitude, double longitude, double height)
{
    mslMap_.insert(qMakePair(qRound64(latitude / kCoordinateScale),
                             qRound64(longitude / kCoordinateScale)),
                   height);
}

int EScout::coordMSL(const QGeoCoordinate& coordinate)
{
    if (!coordinate.isValid()) {
        return 0;
    }
    const auto exact = mslMap_.constFind(qMakePair(
        qRound64(coordinate.latitude() / kCoordinateScale),
        qRound64(coordinate.longitude() / kCoordinateScale)));
    if (exact != mslMap_.cend()) {
        return qRound64(*exact * kMillimetresPerMetre);
    }
    double closestDistance = std::numeric_limits<double>::infinity();
    double closestHeight = 0.0;
    for (const auto& sample : elevations_) {
        const double distance = coordinate.distanceTo(sample.first);
        if (distance < closestDistance) {
            closestDistance = distance;
            closestHeight = sample.second;
        }
    }
    return std::isfinite(closestDistance)
        ? qRound64(closestHeight * kMillimetresPerMetre)
        : 0;
}

void EScout::saveCoordOnLaunch(double latitude, double longitude, double msl)
{
    launchQCoord_ = QGeoCoordinate(latitude, longitude, msl);
    launchCoord_.lat = qRound64(latitude / kCoordinateScale);
    launchCoord_.lon = qRound64(longitude / kCoordinateScale);
    launchCoord_.alt = qRound64(msl * kMillimetresPerMetre);
    launchCoord_.mslAlt = launchCoord_.alt;
    launchCoord_.valid = launchQCoord_.isValid();
    launchMsl_ = launchCoord_.mslAlt;
    currentGroundMsl_ = launchMsl_;
    lastSavedCoordinateTime_ = nowMs();
}

int EScout::elevationsCoordMLS(const QGeoCoordinate& coordinate)
{
    return coordMSL(coordinate);
}

void EScout::calculateTemporaryOffset(const QGeoCoordinate& coordinate,
                                      QGeoCoordinate& output)
{
    output = coordinate;
    if (!coordinate.isValid() || !hasRoadOffset_) {
        return;
    }
    output = output.atDistanceAndAzimuth(roadOffset_.offsetY, 0.0);
    output = output.atDistanceAndAzimuth(roadOffset_.offsetX, 90.0);
}

void EScout::setTimeOffset(qlonglong offset, int roundDt)
{
    timeOffset_ = offset;
    roundTrip_ = roundDt;
    emit timeSynced();
}

void EScout::setCopterThrottle(int throttle) { copterThrottle_ = throttle; }

void EScout::setPosition(uint32_t timeBootMs,
                         int32_t lat,
                         int32_t lon,
                         int32_t alt,
                         int32_t mslAlt,
                         int16_t vx,
                         int16_t vy,
                         int16_t vz,
                         uint16_t hdg)
{
    if (timeOffset_ == 0) {
        return;
    }
    const int index = appendRing(positions_, positionIndex_,
                                 positionInitialized_);
    EPosition& value = positions_[index];
    value.type = mainGpsState_;
    value.lat = lat;
    value.lon = lon;
    value.alt = alt;
    value.mslAlt = mslAlt;
    value.valid = true;
    value.vx = vx;
    value.vy = vy;
    value.vz = vz;
    value.time = mav2SysTime(timeBootMs);
    value.azimuth = static_cast<float>(hdg) / 100.0F;
    value.groundSpeed =
        std::hypot(static_cast<float>(vx), static_cast<float>(vy)) / 100.0F;
    value.groundCourse = value.azimuth;
    rawImuPositions_[index] = value;
    imuPositions_[index] = value;
    if (launchMsl_ == 0 && mslAlt != 0) {
        currentGroundMsl_ = mslAlt - alt;
    }
    if (!takeoffed_ && alt > 30000) {
        takeoffed_ = true;
        emit takeoffed();
    }
}

void EScout::setRawPositionEvent(uint32_t timeBootMs,
                                 int32_t lat,
                                 int32_t lon,
                                 int32_t alt)
{
    if (timeOffset_ == 0) {
        return;
    }
    const int index = appendRing(rawGpsPositions_, rawGpsIndex_,
                                 rawGpsInitialized_);
    ERawGpsPosition& value = rawGpsPositions_[index];
    value.lat = lat;
    value.lon = lon;
    value.alt = alt;
    value.mslAlt = alt;
    value.time = mav2SysTime(timeBootMs);
}

void EScout::setAttitude(uint32_t timeBootMs,
                         float pitch,
                         float yaw,
                         float roll,
                         float pitchSpeed,
                         float yawSpeed,
                         float rollSpeed)
{
    if (timeOffset_ == 0) {
        return;
    }
    const int index = appendRing(attitudes_, attitudeIndex_,
                                 attitudeInitialized_);
    EAttitude& value = attitudes_[index];
    value.pitch = pitch * kRadiansToDegrees;
    value.yaw = normalize360(yaw * kRadiansToDegrees);
    value.roll = roll * kRadiansToDegrees;
    value.pitchSpeed = pitchSpeed * kRadiansToDegrees / 1000.0F;
    value.yawSpeed = yawSpeed * kRadiansToDegrees / 1000.0F;
    value.rollSpeed = rollSpeed * kRadiansToDegrees / 1000.0F;
    value.futurePitchSpeed = value.pitchSpeed;
    value.futureRollSpeed = value.rollSpeed;
    value.futureYaw = value.yaw;
    value.time = mav2SysTime(timeBootMs);
}

void EScout::setBackupAttitude(unsigned char linkId,
                               uint32_t timeBootMs,
                               float pitch,
                               float yaw,
                               float roll,
                               float pitchSpeed,
                               float yawSpeed,
                               float rollSpeed)
{
    EAttitude value;
    value.pitch = pitch * kRadiansToDegrees;
    value.yaw = normalize360(yaw * kRadiansToDegrees);
    value.roll = roll * kRadiansToDegrees;
    value.pitchSpeed = pitchSpeed * kRadiansToDegrees / 1000.0F;
    value.yawSpeed = yawSpeed * kRadiansToDegrees / 1000.0F;
    value.rollSpeed = rollSpeed * kRadiansToDegrees / 1000.0F;
    value.futurePitchSpeed = value.pitchSpeed;
    value.futureRollSpeed = value.rollSpeed;
    value.futureYaw = value.yaw;
    value.time = timeOffset_ == 0 ? nowMs() - 9 : mav2SysTime(timeBootMs);
    backupAttitudes_.insert(linkId, value);
}

void EScout::setAhrsError(float rpError, float yawError)
{
    ahrsRpError_ = rpError;
    ahrsYawError_ = yawError;
    ahrsErrorTime_ = nowMs();
}

void EScout::setDistanceSensor(unsigned int minDist,
                               unsigned int maxDist,
                               unsigned int curDist)
{
    distanceSensorMin_ = minDist;
    distanceSensorMax_ = maxDist;
    distanceSensorValue_ = curDist;
    distanceSensorLastTime_ = nowMs();
    if (distanceSensorUsageMode_ != DISTANCE_SENSOR_NOT_USE &&
        curDist >= minDist && curDist <= maxDist &&
        (!isDiving_ || distanceSensorDivingEnabled_)) {
        const int measuredAltitude = static_cast<int>(curDist) * 10;
        const int currentAltitude = positionIndex_ < 0
            ? measuredAltitude
            : positions_[positionIndex_].alt;
        const int target = measuredAltitude - currentAltitude;
        if (lerpDistanceSensorOffset_) {
            distanceSensorOffset_ =
                qRound(0.8 * distanceSensorOffset_ + 0.2 * target);
        } else {
            distanceSensorOffset_ = target;
        }
        lastRecalcPositionsTime_ = distanceSensorLastTime_;
    }
}

void EScout::launchAcceleration()
{
    if (!takeoffed_) {
        takeoffed_ = true;
        emit takeoffed();
    }
}
void EScout::setManuTakeoffed() { launchAcceleration(); }
void EScout::setSmoothAttSpeed(bool enabled)
{
    smoothAttitudeSpeed_ = enabled;
}

void EScout::landed()
{
    takeoffed_ = false;
    distanceSensorOffset_ = 0;
    targetAltitudeOffset_ = 0;
    resetYawOffset();
}

void EScout::setEPosition(qlonglong time,
                          unsigned char status,
                          unsigned char satellites,
                          int lat,
                          int lon,
                          int alt,
                          float vx,
                          float vy,
                          float vz,
                          float groundSpeed,
                          float groundCourse)
{
    const int index = appendRing(ePositions_, ePositionIndex_,
                                 ePositionInitialized_);
    EPosition& value = ePositions_[index];
    value.type = status > 1 || satellites > 2 ? 2 : 0;
    value.lat = lat;
    value.lon = lon;
    value.alt = alt * 10;
    value.mslAlt = value.alt + currentGroundMsl_;
    value.valid = value.type == 2;
    value.vx = static_cast<std::int16_t>(std::clamp(
        std::lround(vx * 100.0F),
        static_cast<long>(std::numeric_limits<std::int16_t>::min()),
        static_cast<long>(std::numeric_limits<std::int16_t>::max())));
    value.vy = static_cast<std::int16_t>(std::clamp(
        std::lround(vy * 100.0F),
        static_cast<long>(std::numeric_limits<std::int16_t>::min()),
        static_cast<long>(std::numeric_limits<std::int16_t>::max())));
    value.vz = static_cast<std::int16_t>(std::clamp(
        std::lround(vz * 100.0F),
        static_cast<long>(std::numeric_limits<std::int16_t>::min()),
        static_cast<long>(std::numeric_limits<std::int16_t>::max())));
    value.groundSpeed = clampFinite(groundSpeed, 0.0F, 100.0F);
    value.groundCourse = normalize360(groundCourse);
    value.azimuth = value.groundCourse;
    value.time = time;
    eLinkSatelliteStatus_ = status;
    eLinkSatelliteCount_ = satellites;
    eGroundSpeed_ = value.groundSpeed;
    lastELinkEventTime_ = nowMs();
}

void EScout::setEBackupPosition(qlonglong time,
                                unsigned char status,
                                unsigned char satellites,
                                int lat,
                                int lon,
                                int alt,
                                float vx,
                                float vy,
                                float vz,
                                float groundSpeed,
                                float groundCourse)
{
    const int index = appendRing(eBackupPositions_, eBackupPositionIndex_,
                                 eBackupPositionInitialized_);
    EPosition& value = eBackupPositions_[index];
    value.type = status > 1 || satellites > 2 ? 2 : 0;
    value.lat = lat;
    value.lon = lon;
    value.alt = alt * 10;
    value.mslAlt = value.alt + currentGroundMsl_;
    value.valid = value.type == 2;
    value.vx = static_cast<std::int16_t>(clampFinite(
        vx * 100.0F, std::numeric_limits<std::int16_t>::min(),
        std::numeric_limits<std::int16_t>::max()));
    value.vy = static_cast<std::int16_t>(clampFinite(
        vy * 100.0F, std::numeric_limits<std::int16_t>::min(),
        std::numeric_limits<std::int16_t>::max()));
    value.vz = static_cast<std::int16_t>(clampFinite(
        vz * 100.0F, std::numeric_limits<std::int16_t>::min(),
        std::numeric_limits<std::int16_t>::max()));
    value.groundSpeed = clampFinite(groundSpeed, 0.0F, 100.0F);
    value.groundCourse = normalize360(groundCourse);
    value.azimuth = value.groundCourse;
    value.time = time;
    eLinkBackupSatelliteCount_ = satellites;
    lastELinkEventTime_ = nowMs();
}

void EScout::setEAttitude(qlonglong time,
                          float pitch,
                          float roll,
                          float yaw)
{
    const int index = appendRing(eAttitudes_, eAttitudeIndex_,
                                 eAttitudeInitialized_);
    EAttitude& value = eAttitudes_[index];
    value.pitch = pitch;
    value.roll = roll;
    value.yaw = normalize360(yaw);
    value.futureYaw = value.yaw;
    value.time = time;
    if (index != 0 || eAttitudes_[(index - 1 + kHistorySize) % kHistorySize].time != 0) {
        const int previous = (index - 1 + kHistorySize) % kHistorySize;
        const qlonglong elapsed = value.time - eAttitudes_[previous].time;
        if (elapsed > 0) {
            value.pitchSpeed =
                (value.pitch - eAttitudes_[previous].pitch) / elapsed;
            value.rollSpeed = (value.roll - eAttitudes_[previous].roll) / elapsed;
            value.yawSpeed = signedAngularDelta(eAttitudes_[previous].yaw,
                                                value.yaw) /
                elapsed;
            value.futurePitchSpeed = value.pitchSpeed;
            value.futureRollSpeed = value.rollSpeed;
        }
    }
    lastELinkEventTime_ = nowMs();
}

void EScout::setExactMagOffsetEnabled(bool enabled)
{
    exactMagOffsetEnabled_ = enabled;
}
void EScout::setExactMagTargetEnabled(bool enabled)
{
    exactMagTargetEnabled_ = enabled;
}
void EScout::setCurrentExactMagValue(int value)
{
    exactMagValue_ = value;
    exactMagValueValid_ = true;
}
void EScout::setExactMagDisableTime(qlonglong time)
{
    exactMagDisableTime_ = time;
}
void EScout::setExactMagIsFreezed(bool freezed)
{
    exactMagFreezed_ = freezed;
}
void EScout::exactMagReseted()
{
    exactMagOffsets_.clear();
    exactMagValueValid_ = false;
}

void EScout::setCopterAttitude(uint32_t timeBootMs,
                               float pitch,
                               float yaw,
                               float roll,
                               float pitchSpeed,
                               float yawSpeed,
                               float rollSpeed)
{
    isCopter_ = true;
    setAttitude(timeBootMs, pitch, yaw, roll, pitchSpeed, yawSpeed, rollSpeed);
}

void EScout::setCopterPosition(uint32_t timeBootMs,
                               int32_t lat,
                               int32_t lon,
                               int32_t alt,
                               int32_t mslAlt,
                               int16_t vx,
                               int16_t vy,
                               int16_t vz,
                               uint16_t hdg)
{
    isCopter_ = true;
    setPosition(timeBootMs, lat, lon, alt, mslAlt, vx, vy, vz, hdg);
}

void EScout::calibrateRollPitch()
{
    if (attitudeIndex_ < 0 || eAttitudeIndex_ < 0) {
        return;
    }
    calibrationPitchOffset_ = eAttitudes_[eAttitudeIndex_].pitch -
        attitudes_[attitudeIndex_].pitch;
    calibrationRollOffset_ = eAttitudes_[eAttitudeIndex_].roll -
        attitudes_[attitudeIndex_].roll;
    rollPitchCalibrated_ = true;
}

void EScout::calibrateYaw()
{
    if (attitudeIndex_ < 0 || eAttitudeIndex_ < 0) {
        return;
    }
    calibrationYawOffset_ = signedAngularDelta(
        attitudes_[attitudeIndex_].yaw, eAttitudes_[eAttitudeIndex_].yaw);
    yawCalibrated_ = true;
}

void EScout::resetYawOffset()
{
    yawOffset_ = 0.0F;
    yawOffsetTime_ = 0;
    yawOffsetLifetimeMs_ = 7000;
}

float EScout::currentYawOffset()
{
    if (!yawOffsetEnabled_ || yawOffsetTime_ == 0 || yawOffset_ == 0.0F) {
        return 0.0F;
    }
    const qlonglong age = std::max<qlonglong>(0, nowMs() - yawOffsetTime_);
    const qlonglong halfLife = yawOffsetLifetimeMs_ / 2;
    if (age <= halfLife) {
        return yawOffset_;
    }
    if (age >= yawOffsetLifetimeMs_) {
        return 0.0F;
    }
    return yawOffset_ * static_cast<float>(yawOffsetLifetimeMs_ - age) /
        static_cast<float>(std::max<qlonglong>(1, halfLife));
}

void EScout::setELinkRCControlState(bool enabled)
{
    eLinkRcControlState_ = enabled;
}
void EScout::setPTZDistSensorPitch(float pitch)
{
    ptzDistanceSensorPitch_ = pitch;
}
void EScout::setDebugCorrectAltByPitch(bool enabled)
{
    debugCorrectAltitudeByPitch_ = enabled;
}

void EScout::setRoadOffset(const RoadValidateResult& result)
{
    previousRoadOffset_ = roadOffset_;
    roadOffset_ = result;
    hasRoadOffset_ = true;
    emit roadOffsetAccepted(roadOffset_);
}

void EScout::setLerpDistSensorOffset(bool enabled)
{
    lerpDistanceSensorOffset_ = enabled;
}

void EScout::selfDiagnostic()
{
    if (currentState() != 0 && attitudeIndex_ >= 0) {
        emit valid();
    } else {
        emit invalid();
    }
}

void EScout::setCurrentPosFromTelemetry(qlonglong time, int lat, int lon)
{
    if (positionIndex_ < 0) {
        return;
    }
    const QGeoCoordinate before = coordinateOf(positions_[positionIndex_]);
    positions_[positionIndex_].lat = lat;
    positions_[positionIndex_].lon = lon;
    imuPositions_[positionIndex_] = positions_[positionIndex_];
    lastVNav2TrackingTime_ = time;
    const QGeoCoordinate after = coordinateOf(positions_[positionIndex_]);
    emit imuCorrected(1.0F,
                      before.isValid() && after.isValid()
                          ? static_cast<float>(before.distanceTo(after))
                          : 0.0F,
                      -1,
                      IMU_CORRECT_TELEMETRY);
}

void EScout::setCurrentPosFromVNav(qlonglong time,
                                   int lat,
                                   int lon,
                                   float confidence)
{
    if (positionIndex_ < 0 || vnavIgnored_) {
        return;
    }
    EPosition& value = positions_[positionIndex_];
    const QGeoCoordinate previous = coordinateOf(value);
    const QGeoCoordinate target(static_cast<double>(lat) * kCoordinateScale,
                                static_cast<double>(lon) * kCoordinateScale,
                                previous.altitude());
    if (previous.isValid() && target.isValid()) {
        lastVNavCorrectDistance_ = previous.distanceTo(target);
        lastVNavCorrectAzimuth_ = previous.azimuthTo(target);
        const float factor = clampFinite(confidence, 0.0F, 1.0F) * inertCoeff_;
        value.lat = qRound64(value.lat + (lat - value.lat) * factor);
        value.lon = qRound64(value.lon + (lon - value.lon) * factor);
        vnavAzimuth_ = lastVNavCorrectAzimuth_;
    }
    lastVNavCorrectTime_ = time;
    lastVNav2TrackingTime_ = time;
}

void EScout::setCurrentPosFromVNav(qlonglong time, int lat, int lon)
{
    setCurrentPosFromVNav(time, lat, lon, 1.0F);
}
void EScout::setCurrentPosFromImageTarget(qlonglong time, int lat, int lon)
{
    setCurrentPosFromVNav(time, lat, lon, 1.0F);
}
void EScout::setCurrentPosFromWayTargets(qlonglong time, int lat, int lon)
{
    setCurrentPosFromVNav(time, lat, lon, 1.0F);
}
void EScout::setCurrentPosFromFlower(qlonglong time, int lat, int lon)
{
    setCurrentPosFromVNav(time, lat, lon, 1.0F);
}

void EScout::setCurrentAHRSFromVNav(qlonglong time,
                                    int lat,
                                    int lon,
                                    unsigned int alt,
                                    float roll,
                                    float pitch,
                                    float yaw,
                                    unsigned char status)
{
    Q_UNUSED(lat)
    Q_UNUSED(lon)
    Q_UNUSED(alt)
    lastVNav2FixTime_ = time;
    if (status == 0 || attitudeIndex_ < 0 || vnavIgnored_) {
        return;
    }
    const EAttitude& current = attitudes_[attitudeIndex_];
    calibrationRollOffset_ = roll - current.roll;
    calibrationPitchOffset_ = pitch - current.pitch;
    calibrationYawOffset_ = signedAngularDelta(current.yaw, yaw);
    rollPitchCalibrated_ = true;
    yawCalibrated_ = true;
    emit imuCorrected(1.0F, calibrationYawOffset_, -1,
                      IMU_CORRECT_AHRS);
}

void EScout::setYawOffsetFromVNav(qlonglong time, float offset)
{
    if (!yawOffsetEnabled_ || !std::isfinite(offset)) {
        return;
    }
    const qlonglong age = yawOffsetTime_ == 0
        ? yawOffsetLifetimeMs_
        : std::max<qlonglong>(0, time - yawOffsetTime_);
    yawOffsetLifetimeMs_ = static_cast<int>(
        std::clamp<qlonglong>(age, 7000, 20000));
    float limit = 12.0F;
    if (yawOffsetLifetimeMs_ > 16000) {
        limit = 20.0F;
    } else if (yawOffsetLifetimeMs_ > 12000) {
        limit = 16.0F;
    }
    if (yawOffset_ * offset > 0.0F) {
        offset *= 0.5F;
    }
    yawOffset_ = std::clamp(yawOffset_ + offset, -limit, limit);
    yawOffsetTime_ = time;
}

void EScout::setEnabledYawOffset(bool enabled)
{
    yawOffsetEnabled_ = enabled;
}

void EScout::undoLastRoadOffset()
{
    std::swap(roadOffset_, previousRoadOffset_);
    hasRoadOffset_ = roadOffset_.time != 0 || roadOffset_.accepted;
}
void EScout::setVNavIgnored(bool ignore) { vnavIgnored_ = ignore; }

QGeoCoordinate EScout::lastRawIMU()
{
    if (positionIndex_ < 0 || rawImuPositions_[positionIndex_].time == 0) {
        return lastGpsCoord();
    }
    return coordinateOf(rawImuPositions_[positionIndex_]);
}

void EScout::saveCoordinateByIMU()
{
    savedCoordinateByImu_ = lastRawIMU();
    hasSavedCoordinateByImu_ = savedCoordinateByImu_.isValid();
    lastSavedCoordinateTime_ = nowMs();
}

void EScout::restoreCoordinateByIMU()
{
    if (!hasSavedCoordinateByImu_ || positionIndex_ < 0) {
        return;
    }
    positions_[positionIndex_].lat =
        qRound64(savedCoordinateByImu_.latitude() / kCoordinateScale);
    positions_[positionIndex_].lon =
        qRound64(savedCoordinateByImu_.longitude() / kCoordinateScale);
}

void EScout::setReplaceYawFromExactMag(bool enabled)
{
    replaceYawFromExactMag_ = enabled;
}
void EScout::exactMagInited() { exactMagInitialized_ = true; }
void EScout::setInertCoeff(float coefficient)
{
    inertCoeff_ = clampFinite(coefficient, 0.0F, 1.0F, 0.8F);
}

bool EScout::vnavAzimuth(float& azimuth)
{
    azimuth = std::max(0.0F, vnavAzimuth_);
    return false;
}

void EScout::enableOdometry()
{
    odometryEnabled_ = true;
    upConnectOdometry();
}

void EScout::upConnectOdometry()
{
    if (!odometryEnabled_ || odometryPort_ <= 0) {
        return;
    }
    if (!odometrySocket_) {
        odometrySocket_ = new QUdpSocket(this);
        connect(odometrySocket_, &QUdpSocket::readyRead, this,
                &EScout::readOdoPendingDatagrams);
    }
    if (odometrySocket_->state() != QAbstractSocket::BoundState) {
        odometrySocket_->bind(QHostAddress::AnyIPv4,
                              static_cast<quint16>(odometryPort_),
                              QUdpSocket::ShareAddress |
                                  QUdpSocket::ReuseAddressHint);
    }
}

void EScout::readOdoPendingDatagrams()
{
    if (!odometrySocket_) {
        return;
    }
    while (odometrySocket_->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(odometrySocket_->pendingDatagramSize()));
        odometrySocket_->readDatagram(datagram.data(), datagram.size());
    }
}

void EScout::updateCurMSL()
{
    const QGeoCoordinate coordinate = lastGpsCoord();
    const int value = coordMSL(coordinate);
    if (value != 0) {
        currentGroundMsl_ = value;
    }
}

void EScout::autoUpdateExactMagOffset()
{
    if (!exactMagOffsetEnabled_ || exactMagFreezed_ ||
        nowMs() < exactMagDisableTime_) {
        return;
    }
    calcExactMagOffsetByMainMag();
}

void EScout::calcExactMagOffsetByMainMag()
{
    if (!exactMagValueValid_ || attitudeIndex_ < 0) {
        return;
    }
    exactMagOffsets_.append(signedAngularDelta(
        attitudes_[attitudeIndex_].yaw, static_cast<float>(exactMagValue_)));
    while (exactMagOffsets_.size() > 100) {
        exactMagOffsets_.removeFirst();
    }
}

void EScout::readExactMagTable()
{
    exactMagOffsets_.clear();
    if (elevationsPath_.isEmpty()) {
        return;
    }
    QFile file(elevationsPath_ + QStringLiteral("/exact_mag_offsets.txt"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    QTextStream input(&file);
    while (!input.atEnd()) {
        bool ok = false;
        const float value = input.readLine().trimmed().toFloat(&ok);
        if (ok && std::isfinite(value)) {
            exactMagOffsets_.append(value);
        }
    }
}

ECoord EScout::lastGpsECoord()
{
    if (mainGpsState_ == 1 && positionIndex_ >= 0) {
        return coordPrefix(positions_[positionIndex_]);
    }
    if (gpsUsageMode_ != GPS_NOT_USE && ePositionIndex_ >= 0) {
        return coordPrefix(ePositions_[ePositionIndex_]);
    }
    return {};
}

QGeoCoordinate EScout::lastGpsCoord()
{
    return coordinateOf(lastGpsECoord());
}

void EScout::timerEvent(QTimerEvent* event)
{
    Q_UNUSED(event)
    selfDiagnostic();
}
