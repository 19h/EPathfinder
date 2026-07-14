#include "pathfinder/pathfinder.hpp"

#include "scout/escout.hpp"

#include <QDateTime>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

template <typename T, std::size_t N>
void assignChannels(std::array<T, N>& destination,
                    const std::array<T, N>& source)
{
    destination = source;
}

} // namespace

std::int32_t EPathfinderParams::vehicleVersion() const noexcept
{
    std::int32_t value{};
    static_assert(0xD4U + sizeof(value) <= 0x108U);
    std::memcpy(&value, scalarPrefix.data() + 0xD4U, sizeof(value));
    return value;
}

void EPathfinderParams::setVehicleVersion(const std::int32_t value) noexcept
{
    std::memcpy(scalarPrefix.data() + 0xD4U, &value, sizeof(value));
}

QString mode2string(const PathfinderMode mode)
{
    switch (mode) {
    case PathfinderMode::Disabled: return QStringLiteral("DISABLED");
    case PathfinderMode::Test: return QStringLiteral("TEST");
    case PathfinderMode::Launch: return QStringLiteral("LAUNCH");
    case PathfinderMode::Plan: return QStringLiteral("PLAN");
    case PathfinderMode::ManualPlan: return QStringLiteral("MANU_PLAN");
    case PathfinderMode::FindLine: return QStringLiteral("FIND_LINE");
    case PathfinderMode::FindCircle: return QStringLiteral("FIND_CIRCLE");
    case PathfinderMode::FindQuad: return QStringLiteral("FIND_QUAD");
    case PathfinderMode::TargetPositionCorrection:
        return QStringLiteral("TRG_POS_CORRECTION");
    case PathfinderMode::Target: return QStringLiteral("TARGET");
    case PathfinderMode::Rescue: return QStringLiteral("RESCUE");
    case PathfinderMode::Interceptor: return QStringLiteral("INTERCEPTOR");
    case PathfinderMode::CopterFindLine: return QStringLiteral("COPTER_FIND_LINE");
    case PathfinderMode::CopterFindPoint: return QStringLiteral("COPTER_FIND_POINT");
    case PathfinderMode::CopterManualPlan: return QStringLiteral("COPTER_MANU_PLAN");
    case PathfinderMode::CopterTarget: return QStringLiteral("COPTER_TARGET");
    case PathfinderMode::ManualLand: return QStringLiteral("MANU_LAND");
    case PathfinderMode::Parade: return QStringLiteral("PARADE");
    case PathfinderMode::CopterTakeoff: return QStringLiteral("COPTER_TAKEOFF");
    case PathfinderMode::Spy1: return QStringLiteral("SPY1");
    case PathfinderMode::Spy2: return QStringLiteral("SPY2");
    }
    return QStringLiteral("UNKNOWN");
}

EPathfinder::EPathfinder(EScout* const scout,
                         const EPathfinderParams& params,
                         QObject* const parent)
    : QObject(parent != nullptr ? parent : scout)
    , scout_(scout)
    , params_(params)
    , vehicleVersion_(params.vehicleVersion())
{
    qRegisterMetaType<EPathfinderParams>();
    qRegisterMetaType<PathfinderMode>();
    qRegisterMetaType<LaunchMode>();

    if (scout_ != nullptr) {
        connect(scout_, &EScout::valid, this, &EPathfinder::scoutValid);
        connect(scout_, &EScout::invalid, this, &EPathfinder::scoutInvalid);
        connect(scout_, &EScout::roadOffsetAccepted,
                this, &EPathfinder::roadOffsetAccepted);
        connect(scout_, &EScout::imuCorrectedByPeekAtGPS,
                this, &EPathfinder::imuCorrectedByPeekAtGPS);
        connect(scout_, &EScout::imuCorrected,
                this, &EPathfinder::imuCorrected);
    }
}

PathfinderMode EPathfinder::currentMode() const noexcept { return currentMode_; }

void EPathfinder::setCurrentMode(const PathfinderMode mode)
{
    if (currentMode_ == mode) {
        return;
    }
    currentMode_ = mode;
    emit setMode(static_cast<int>(mode));
    if (mode == PathfinderMode::Disabled) {
        emit controlOff();
        emit copterControlOff();
    }
}

void EPathfinder::setLaunchMode(const LaunchMode mode) noexcept { launchMode_ = mode; }
LaunchMode EPathfinder::currentLaunchMode() const noexcept { return launchMode_; }
void EPathfinder::setBurnRocketDelay(const int delayMs) noexcept { burnRocketDelayMs_ = delayMs; }
int EPathfinder::burnRocketDelay() const noexcept { return burnRocketDelayMs_; }

float EPathfinder::airSpeedLastValue() const noexcept
{
    return std::round(airSpeedMetresPerSecond_ * 3.6F);
}

int EPathfinder::vehicleRole() const noexcept
{
    if (vehicleVersion_ == 17) {
        return 2;
    }
    if (vehicleVersion_ > 17) {
        return vehicleVersion_ == 22 ? 2 : 0;
    }
    if (vehicleVersion_ > 12) {
        return vehicleVersion_ == 16 ? 1 : 0;
    }
    return vehicleVersion_ > 10 ? 1 : 0;
}

bool EPathfinder::extremeGimbalTestIsEnabled() const noexcept
{
    return extremeGimbalTestEnabled_;
}

bool EPathfinder::isSeaMode() const noexcept { return seaMode_; }
CameraParams EPathfinder::currentCameraParams() const { return cameraParams_; }

QList<int> EPathfinder::vehicleTypes() const
{
    return {vehicleRole(), vehicleVersion_};
}

void EPathfinder::currentModeChanged(const int newPlaneMode) { Q_UNUSED(newPlaneMode) }
void EPathfinder::copterCurrentModeChanged(const int newCopterMode) { Q_UNUSED(newCopterMode) }

void EPathfinder::currentMissionItemChanged(const int index,
                                            const bool updatePrevious)
{
    Q_UNUSED(updatePrevious)
    currentMissionIndex_ = index;
}

void EPathfinder::currentMissionItemChanged(const int index)
{
    currentMissionItemChanged(index, true);
}

void EPathfinder::homingOn() { emit controlOn(1); }
void EPathfinder::homingOff() { emit controlOff(); }
void EPathfinder::copterHomingOn() { emit copterControlOn(1); }
void EPathfinder::copterHomingOff() { emit copterControlOff(); }
void EPathfinder::backupHomingOn() { emit startBackupControllerDrop(); }
void EPathfinder::backupHomingOff() {}

void EPathfinder::airspeedEvent(const float airSpeed, const float groundSpeed)
{
    airSpeedMetresPerSecond_ = airSpeed;
    groundSpeedMetresPerSecond_ = groundSpeed;
    if (scout_ != nullptr) {
        scout_->setEIMUAirspeed(airSpeed);
    }
    emit sendAirspeedToELink(airSpeed);
}

void EPathfinder::windEvent(const float direction, const float speed,
                            const float verticalSpeed)
{
    Q_UNUSED(verticalSpeed)
    setWind(Wind{direction, speed});
}

void EPathfinder::setTestControlParams(const int testRoll, const int testPitch,
                                       const int throttle, const int throttle2,
                                       const int cameraRoll, const int cameraPitch,
                                       const int cameraYaw, const int cameraZoom,
                                       const bool cameraCenter)
{
    Q_UNUSED(throttle2)
    Q_UNUSED(cameraRoll)
    Q_UNUSED(cameraYaw)
    Q_UNUSED(cameraCenter)
    emit setControlParams(testRoll, 0, testPitch, throttle);
    emit setCameraParams(cameraPitch);
    emit setZoom(static_cast<float>(cameraZoom));
}

void EPathfinder::emptyDTP() {}

void EPathfinder::targetDetected(const CameraParams& cameraParams,
                                 const QList<Target>& targets)
{
    cameraParams_ = cameraParams;
    Q_UNUSED(targets)
}

void EPathfinder::roadDetected(const qlonglong time,
                               const CameraParams& cameraParams,
                               const QVector<RoadPointAngle>& points,
                               const bool isSingleLine)
{
    Q_UNUSED(time)
    Q_UNUSED(points)
    Q_UNUSED(isSingleLine)
    cameraParams_ = cameraParams;
}

void EPathfinder::imageTargetDetected(const CameraParams& cameraParams,
                                      const Target& target)
{
    cameraParams_ = cameraParams;
    Q_UNUSED(target)
}

void EPathfinder::roadScreenParts(const qlonglong time, const int left,
                                  const int middle, const int right)
{
    roadPartsTime_ = time;
    roadParts_ = {left, middle, right};
    if (left <= 2 && middle <= 2 && right <= 2) {
        dominantRoadPart_ = 3;
    } else if (left > std::max(middle, right)) {
        dominantRoadPart_ = 0;
    } else {
        dominantRoadPart_ = right > std::max(left, middle) ? 2 : 1;
    }
}

void EPathfinder::enableVisionDetect() { updateVisionModuleActivities(); }
void EPathfinder::launchAcceleration() { emit takeoffed(); }
void EPathfinder::setCurrentPlan(const Plan& plan) { plan_ = plan; }

void EPathfinder::setWind(const Wind& globalWind)
{
    wind_ = globalWind;
    if (scout_ != nullptr) {
        scout_->setWind(globalWind);
    }
}

void EPathfinder::setLastWPCoordFromTelemetry(const int lat, const int lon)
{
    Q_UNUSED(lat)
    Q_UNUSED(lon)
}

void EPathfinder::startSearchFromTelemetry(const qlonglong time) { Q_UNUSED(time) }

void EPathfinder::setParams(const EPathfinderParams& params)
{
    params_ = params;
    setVehicleVersion(params.vehicleVersion());
}

void EPathfinder::currentControls(const uint16_t roll, const uint16_t pitch,
                                  const uint16_t throttle)
{
    controls_ = {roll, pitch, throttle};
}

void EPathfinder::copterCurrentControls(const uint16_t roll,
                                        const uint16_t pitch,
                                        const uint16_t throttle)
{
    copterControls_ = {roll, pitch, throttle};
}

uint16_t EPathfinder::rcValue(const int id)
{
    return id >= 5 && id <= 18 ? rcValues_[static_cast<std::size_t>(id - 5)] : 0;
}

void EPathfinder::currentRcValues(const uint16_t c5, const uint16_t c6,
                                  const uint16_t c7, const uint16_t c8,
                                  const uint16_t c9, const uint16_t c10,
                                  const uint16_t c11, const uint16_t c12,
                                  const uint16_t c13, const uint16_t c14,
                                  const uint16_t c15, const uint16_t c16,
                                  const uint16_t c17, const uint16_t c18)
{
    assignChannels(rcValues_, std::array<uint16_t, 14>{
        c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15, c16, c17, c18});
}

void EPathfinder::copterCurrentRcValues(
    const uint16_t c5, const uint16_t c6, const uint16_t c7,
    const uint16_t c8, const uint16_t c9, const uint16_t c10,
    const uint16_t c11, const uint16_t c12, const uint16_t c13,
    const uint16_t c14, const uint16_t c15, const uint16_t c16,
    const uint16_t c17, const uint16_t c18)
{
    assignChannels(copterRcValues_, std::array<uint16_t, 14>{
        c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15, c16, c17, c18});
}

float EPathfinder::getManuMaxAccum() { return manualMaximumAccumulation_; }
qlonglong EPathfinder::getManuOldTimeOffset() { return manualOldTimeOffset_; }
void EPathfinder::activateWeapon() {}
void EPathfinder::cameraParamsChanged(const CameraParams& params) { cameraParams_ = params; }
void EPathfinder::setPTZCamera(const bool isPTZ) { ptzCamera_ = isPTZ; }
void EPathfinder::scoutValid() { scoutValid_ = true; }
void EPathfinder::scoutInvalid() { scoutValid_ = false; }
void EPathfinder::setTestPTZCameraIsCenter(const bool value) { testPtzCenter_ = value; }
void EPathfinder::setPTZCalibration(const bool enable) { ptzCalibration_ = enable; }
void EPathfinder::setVisionTest(const bool enabled) { visionTest_ = enabled; }

void EPathfinder::preprocessingScaleChanged(const float value)
{
    preprocessingScale_ = value;
}

void EPathfinder::objectDetectorVisionModuleEnabled(const bool enabled)
{
    objectDetectorEnabled_ = enabled;
}

void EPathfinder::pathfinderVisionModuleEnabled(const bool enabled)
{
    pathfinderVisionEnabled_ = enabled;
}

void EPathfinder::videoSaverVisionModuleEnabled(const bool enabled)
{
    videoSaverEnabled_ = enabled;
}

void EPathfinder::updateVisionModuleActivities()
{
    emit setPreprocessingScale(preprocessingScale_);
    emit setObjectDetectorVisionModuleEnabled(objectDetectorEnabled_);
    emit setPathfinderVisionModuleEnabled(pathfinderVisionEnabled_);
    emit setVideoSaverVisionModuleEnabled(videoSaverEnabled_);
}

void EPathfinder::setRoadsEnabled(const bool enabled) { roadsEnabled_ = enabled; }
void EPathfinder::roadOffsetItemAdded(const RoadValidateResult& item) { Q_UNUSED(item) }
void EPathfinder::roadOffsetAccepted(const RoadValidateResult& result) { roadOffset_ = result; }

void EPathfinder::imuCorrected(const float confidence, const float offset,
                               const int objectIdx, const IMUCorrectSource source)
{
    Q_UNUSED(confidence)
    Q_UNUSED(offset)
    Q_UNUSED(objectIdx)
    Q_UNUSED(source)
}

void EPathfinder::checkNextWP() {}
void EPathfinder::checkARMWP() {}
void EPathfinder::imuCorrectedByPeekAtGPS() {}
void EPathfinder::spinDetected() {}
void EPathfinder::setRelease(const bool release) { released_ = release; }
void EPathfinder::setBatteryVoltage(const int voltage) { batteryVoltage_ = voltage; }
void EPathfinder::setRadarPlaneTarget(const PlaneTarget& target) { radarTarget_ = target; }
void EPathfinder::setLidarPlaneTarget(const QList<PlaneCoord>& targets) { lidarTargets_ = targets; }

void EPathfinder::setShumodavTarget(const short powerAzimuth,
                                    const short powerElevation,
                                    const short angleAzimuth,
                                    const short angleElevation)
{
    Q_UNUSED(powerAzimuth)
    Q_UNUSED(powerElevation)
    Q_UNUSED(angleAzimuth)
    Q_UNUSED(angleElevation)
}

void EPathfinder::gimbalDeviceAttitudeStatus(
    const qlonglong time, const unsigned char componentId,
    const unsigned char gimbalDeviceId, const float qw, const float qx,
    const float qy, const float qz, const float angularVelocityX,
    const float angularVelocityY, const float angularVelocityZ,
    const float deltaYaw, const float deltaYawVelocity)
{
    Q_UNUSED(time)
    Q_UNUSED(componentId)
    Q_UNUSED(gimbalDeviceId)
    Q_UNUSED(qw)
    Q_UNUSED(qx)
    Q_UNUSED(qy)
    Q_UNUSED(qz)
    Q_UNUSED(angularVelocityX)
    Q_UNUSED(angularVelocityY)
    Q_UNUSED(angularVelocityZ)
    Q_UNUSED(deltaYaw)
    Q_UNUSED(deltaYawVelocity)
}

void EPathfinder::roadsLookAt(const QGeoCoordinate& coordinate) { Q_UNUSED(coordinate) }
void EPathfinder::roadsStopLookAt() {}
void EPathfinder::setVNavEnabled(const bool enabled) { vnavEnabled_ = enabled; }

void EPathfinder::setCurrentPosFromFlower(const int lat, const int lon)
{
    emit setCurrentPos(QDateTime::currentMSecsSinceEpoch(), lat, lon);
}

void EPathfinder::setAHRSFromFlower(const float roll, const float pitch,
                                    const float yaw, const unsigned char gnss,
                                    const int lat, const int lon, const int alt,
                                    const float heading, const float groundSpeed,
                                    const float airSpeed)
{
    Q_UNUSED(gnss)
    Q_UNUSED(lat)
    Q_UNUSED(lon)
    Q_UNUSED(alt)
    Q_UNUSED(heading)
    groundSpeedMetresPerSecond_ = groundSpeed;
    airSpeedMetresPerSecond_ = airSpeed;
    emit sendAHRSToMavlink(QDateTime::currentMSecsSinceEpoch(), roll, pitch, yaw);
}

void EPathfinder::setAHRSFromVNav(const qlonglong time, const int lat,
                                  const int lon, const unsigned int alt,
                                  const float gimbalRoll, const float gimbalPitch,
                                  const float gimbalYaw, const unsigned char status)
{
    Q_UNUSED(lat)
    Q_UNUSED(lon)
    Q_UNUSED(alt)
    Q_UNUSED(status)
    emit sendAHRSToMavlink(time, gimbalRoll, gimbalPitch, gimbalYaw);
}

void EPathfinder::setAHRSByCamera(const qlonglong time, const float vehicleRoll,
                                  const float vehiclePitch, const float vehicleYaw)
{
    emit sendAHRSToMavlink(time, vehicleRoll, vehiclePitch, vehicleYaw);
}

void EPathfinder::rpmValues(const int rpm1, const int rpm2)
{
    Q_UNUSED(rpm1)
    Q_UNUSED(rpm2)
}

void EPathfinder::vehicleDetected(const qlonglong time,
                                  const QList<VehicleTarget>& targets)
{
    Q_UNUSED(time)
    vehicleTargets_ = targets;
}

void EPathfinder::setVehicleVersion(const int value)
{
    vehicleVersion_ = value;
    params_.setVehicleVersion(value);
}

void EPathfinder::setAutoLandEnabled(const bool enabled) { autoLandEnabled_ = enabled; }
void EPathfinder::setInterceptorEnabled(const bool enabled) { interceptorEnabled_ = enabled; }
void EPathfinder::setExtremeGimbalTest(const bool enabled) { extremeGimbalTestEnabled_ = enabled; }
void EPathfinder::setLoadImageTargetsFromClient(const bool value) { loadImageTargetsFromClient_ = value; }
void EPathfinder::setLoadImageMinelayerFromClient(const bool value) { loadImageMinelayerFromClient_ = value; }

void EPathfinder::setLaunchReady(const bool ready)
{
    launchReady_ = ready;
    emit sendTelemetryLaunchReady(ready);
}

void EPathfinder::setBackupBattery(const unsigned char id, const short value)
{
    Q_UNUSED(id)
    Q_UNUSED(value)
}

void EPathfinder::disableControlHandler() { emit controlOff(); }
void EPathfinder::setDisableVehicleMode() { emit setMode(0); }
void EPathfinder::setCircleMode() { emit setMode(1); }
void EPathfinder::setStabilizeMode() { emit setMode(0); }
void EPathfinder::rebootPTZCameraIfCalm() { rebootPTZCamera(); }

void EPathfinder::rebootPTZCamera()
{
    emit ptzCameraOff();
    emit ptzCameraOn();
}

void EPathfinder::enablePTZCamera() { emit ptzCameraOn(); }
void EPathfinder::disablePTZCamera() { emit ptzCameraOff(); }
void EPathfinder::updatePreprocessingScale() { emit setPreprocessingScale(preprocessingScale_); }
void EPathfinder::switchRoadDir() {}
