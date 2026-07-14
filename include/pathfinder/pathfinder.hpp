#pragma once

#include "camera/camera_types.hpp"
#include "elink/elink_telemetry.hpp"
#include "lidar/interceptor_lidar.hpp"
#include "mavlink/plan_handler.hpp"
#include "scout/scout_types.hpp"
#include "vision/vision_types.hpp"
#include "vnav/vnav.hpp"

#include <QGeoCoordinate>
#include <QList>
#include <QMetaType>
#include <QObject>
#include <QPoint>
#include <QStringList>
#include <QVector>

#include <array>
#include <cstdint>

class EScout;

enum class PathfinderMode : std::int32_t {
    Disabled = 0,
    Test = 1,
    Launch = 2,
    Plan = 3,
    ManualPlan = 4,
    FindLine = 5,
    FindCircle = 6,
    FindQuad = 7,
    TargetPositionCorrection = 8,
    Target = 9,
    Rescue = 10,
    Interceptor = 11,
    CopterFindLine = 12,
    CopterFindPoint = 13,
    CopterManualPlan = 14,
    CopterTarget = 15,
    ManualLand = 16,
    Parade = 21,
    CopterTakeoff = 23,
    Spy1 = 24,
    Spy2 = 25,
};

enum class LaunchMode : std::int32_t {
    Mode0 = 0,
    Mode1 = 1,
    Mode2 = 2,
};

QString mode2string(PathfinderMode mode);

// setParams() reads through offset 0x140.  Only one non-trivial member is
// visible in the recovered value: a QString at 0x108.  The erased scalar
// names remain unknown, so their byte-exact storage is retained explicitly.
struct EPathfinderParams {
    std::array<std::uint8_t, 0x108> scalarPrefix{};
    QString stringAt108;
    std::array<std::uint8_t, 0x38> scalarSuffix{};

    std::int32_t vehicleVersion() const noexcept;
    void setVehicleVersion(std::int32_t value) noexcept;
};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
static_assert(offsetof(EPathfinderParams, stringAt108) == 0x108U);
static_assert(offsetof(EPathfinderParams, scalarSuffix) == 0x110U);
static_assert(sizeof(EPathfinderParams) == 0x148U);
#endif

Q_DECLARE_METATYPE(PathfinderMode)
Q_DECLARE_METATYPE(LaunchMode)
Q_DECLARE_METATYPE(EPathfinderParams)

class EPathfinder final : public QObject {
    Q_OBJECT

public:
    explicit EPathfinder(EScout* scout,
                         const EPathfinderParams& params = {},
                         QObject* parent = nullptr);
    ~EPathfinder() override = default;

    PathfinderMode currentMode() const noexcept;
    void setCurrentMode(PathfinderMode mode);
    void setLaunchMode(LaunchMode mode) noexcept;
    LaunchMode currentLaunchMode() const noexcept;
    void setBurnRocketDelay(int delayMs) noexcept;
    int burnRocketDelay() const noexcept;
    float airSpeedLastValue() const noexcept;
    int vehicleRole() const noexcept;
    bool extremeGimbalTestIsEnabled() const noexcept;
    bool isSeaMode() const noexcept;
    CameraParams currentCameraParams() const;
    QList<int> vehicleTypes() const;

signals:
    void controlOn(int mode);
    void controlOff();
    void copterControlOn(int mode);
    void copterControlOff();
    void setControlParams(int roll, int yaw, int pitch, int throttle);
    void setFlapsValue(int value);
    void setCopterControlParams(int roll, int yaw, int pitch, int throttle);
    void setCameraParams(int pitch);
    void resyncMavTimer();
    void resyncCopterMavTimer();
    void setMode(int mode);
    void testEnded();
    void showMessageOnOSD(const QString& message);
    void mavlinkConnected();
    void mavlinkDisconnected();
    void reconnectMavlink();
    void elinkConnected();
    void elinkDisconnected();
    void enableAccelLog();
    void disableAccelLog();
    void parachuteOn();
    void landed();
    void setGimbal(const Gimbal& gimbal);
    void setZoom(float percent);
    void setFocus(float distance, float zoomPercent, int timeoutMSec);
    void ptzCameraOn();
    void ptzCameraOff();
    void takeoffed();
    void armOn();
    void setPreprocessingScale(float value);
    void setObjectDetectorVisionModuleEnabled(bool enabled);
    void setPathfinderVisionModuleEnabled(bool enabled);
    void setVideoSaverVisionModuleEnabled(bool enabled);
    void sendTelemetryPosAtt(qlonglong time, int lat, int lon, int alt,
                             float roll, float pitch, float yaw, float airSpeed);
    void sendTelemetryCameraParams(qlonglong time, float gimbalRoll,
                                   float gimbalPitch, float gimbalYaw);
    void sendTelemetryGPSPos(qlonglong time, int lat, int lon, int alt);
    void sendTelemetryPlan(qlonglong time, const Plan& plan);
    void sendTelemetryLastWPPos(qlonglong time, int lat, int lon);
    void sendTelemetryGPSStatus(qlonglong time, unsigned char epStatus,
                                unsigned char gpsStatus, unsigned char satellites);
    void sendTelemetryCurrentFlightMode(qlonglong time, unsigned char mode);
    void sendTelemetryCurrentMode(qlonglong time, unsigned char mode,
                                  unsigned short targetAltitude);
    void sendTelemetryPlanModeInfo(qlonglong time, unsigned char nextWP,
                                   unsigned short nextWPDistance, float angle);
    void sendTelemetryFindModeInfo(qlonglong time, unsigned char direction,
                                   unsigned short centerDistance);
    void sendTelemetryTargetCorrectionModeInfo(qlonglong time, int targetLat,
                                                int targetLon, unsigned short count,
                                                unsigned short distance, float angle);
    void sendTelemetryTargetModeInfo(qlonglong time, int targetLat, int targetLon,
                                     unsigned char state, unsigned short count,
                                     unsigned short distance, float angle);
    void sendTelemetryDistanceInfo(qlonglong time, int battery,
                                   int homeDistance, int distanceTraveled);
    void sendTelemetryExplorationInfo(qlonglong time, unsigned short windDirection,
                                      float windSpeed);
    void sendTelemetryRadarEmulationInfo(qlonglong time, int lat, int lon, int alt,
                                         float azimuth, float groundSpeed);
    void sendTelemetryToVNav(int lat, int lon, int alt, float yaw,
                             short vx, short vy, float gimbalRoll,
                             float gimbalPitch, float gimbalYaw, float zoom,
                             int gpsLat, int gpsLon);
    void sendTelemetryLaunchReady(bool ready);
    void sendTelemetryLaunched();
    void sendAirspeedToELink(float value);
    void sendVNavStart();
    void sendVNavStop();
    void sendVNavReset();
    void sendVNavMode(unsigned char mode, int targetLat, int targetLon);
    void sendVNavRoadPoints(qlonglong time, const QList<QPoint>& points);
    void sendTVisStart();
    void sendTVisStop();
    void setPTZDistSensorPitch(float pitch);
    void burnRocket();
    void separateRocketStage();
    void roadData(const QVector<QGeoCoordinate>& visibleCoords,
                  bool isSingleRoadLine, const QGeoCoordinate& planeCoord,
                  const QGeoCoordinate& planeRawCoord,
                  const QGeoCoordinate& viewCenter, int prevWPPlanIdx,
                  int nextWPPlanIdx, const QGeoCoordinate& prevWPCoord,
                  const QGeoCoordinate& nextWPCoord, int planeAltitude,
                  float planeYaw, float viewAngle, float viewDist,
                  float currentYawOffset, float imuMaxOffset,
                  bool isExactMagValue, bool isObjectAsRoadPoint);
    void setRoadSimpleMode(bool enabled);
    void setFrameText(const QStringList& lines);
    void setTelemetryFullPower();
    void startTelemetryVideoStream();
    void egimbalValues(float roll, float pitch, float yaw, bool isCenter,
                       qlonglong time);
    void egimbalRollMagValue(qlonglong time, unsigned char componentId,
                             float roll, float pitch, float yaw, float magRoll);
    void egimbalPitchMagValue(qlonglong time, unsigned char componentId,
                              float roll, float pitch, float yaw, float magPitch);
    void egimbalYawMagValue(qlonglong time, unsigned char componentId,
                            float roll, float pitch, float yaw, float magYaw);
    void egimbalMagValues(qlonglong time, unsigned char componentId,
                          float roll, float pitch, float yaw, float magRoll,
                          float magPitch, float magYaw);
    void setCurrentPos(qlonglong sourceTime, int lat, int lon);
    void writeParam(const QString& name, float value);
    void startRpmControl();
    void stopRpmControl();
    void startBackupControllerDrop();
    void openSpyCamera();
    void closeSpyCamera();
    void dropMinelayer();
    void setNormalThrottle();
    void setReverseThrottle();
    void flushLog();
    void sendAHRSToMavlink(qlonglong time, float roll, float pitch, float yaw);

public slots:
    void currentModeChanged(int newPlaneMode);
    void copterCurrentModeChanged(int newCopterMode);
    void currentMissionItemChanged(int index, bool updatePrevious);
    void currentMissionItemChanged(int index);
    void homingOn();
    void homingOff();
    void copterHomingOn();
    void copterHomingOff();
    void backupHomingOn();
    void backupHomingOff();
    void airspeedEvent(float airSpeed, float groundSpeed);
    void windEvent(float direction, float speed, float verticalSpeed);
    void setTestControlParams(int testRoll, int testPitch, int throttle,
                              int throttle2, int cameraRoll, int cameraPitch,
                              int cameraYaw, int cameraZoom, bool cameraCenter);
    void emptyDTP();
    void targetDetected(const CameraParams& cameraParams,
                        const QList<Target>& targets);
    void roadDetected(qlonglong time, const CameraParams& cameraParams,
                      const QVector<RoadPointAngle>& points, bool isSingleLine);
    void imageTargetDetected(const CameraParams& cameraParams,
                             const Target& target);
    void roadScreenParts(qlonglong time, int left, int middle, int right);
    void enableVisionDetect();
    void launchAcceleration();
    void setCurrentPlan(const Plan& plan);
    void setWind(const Wind& globalWind);
    void setLastWPCoordFromTelemetry(int lat, int lon);
    void startSearchFromTelemetry(qlonglong time);
    void setParams(const EPathfinderParams& params);
    void currentControls(uint16_t roll, uint16_t pitch, uint16_t throttle);
    void copterCurrentControls(uint16_t roll, uint16_t pitch, uint16_t throttle);
    uint16_t rcValue(int id);
    void currentRcValues(uint16_t c5, uint16_t c6, uint16_t c7, uint16_t c8,
                         uint16_t c9, uint16_t c10, uint16_t c11, uint16_t c12,
                         uint16_t c13, uint16_t c14, uint16_t c15, uint16_t c16,
                         uint16_t c17, uint16_t c18);
    void copterCurrentRcValues(uint16_t c5, uint16_t c6, uint16_t c7,
                               uint16_t c8, uint16_t c9, uint16_t c10,
                               uint16_t c11, uint16_t c12, uint16_t c13,
                               uint16_t c14, uint16_t c15, uint16_t c16,
                               uint16_t c17, uint16_t c18);
    float getManuMaxAccum();
    qlonglong getManuOldTimeOffset();
    void activateWeapon();
    void cameraParamsChanged(const CameraParams& params);
    void setPTZCamera(bool isPTZ);
    void scoutValid();
    void scoutInvalid();
    void setTestPTZCameraIsCenter(bool isCenter);
    void setPTZCalibration(bool enable);
    void setVisionTest(bool enabled);
    void preprocessingScaleChanged(float value);
    void objectDetectorVisionModuleEnabled(bool enabled);
    void pathfinderVisionModuleEnabled(bool enabled);
    void videoSaverVisionModuleEnabled(bool enabled);
    void updateVisionModuleActivities();
    void setRoadsEnabled(bool enabled);
    void roadOffsetItemAdded(const RoadValidateResult& item);
    void roadOffsetAccepted(const RoadValidateResult& result);
    void imuCorrected(float confidence, float offset, int objectIdx,
                      IMUCorrectSource source);
    void checkNextWP();
    void checkARMWP();
    void imuCorrectedByPeekAtGPS();
    void spinDetected();
    void setRelease(bool release);
    void setBatteryVoltage(int voltage);
    void setRadarPlaneTarget(const PlaneTarget& target);
    void setLidarPlaneTarget(const QList<PlaneCoord>& targets);
    void setShumodavTarget(short powerAzimuth, short powerElevation,
                           short angleAzimuth, short angleElevation);
    void gimbalDeviceAttitudeStatus(qlonglong time, unsigned char componentId,
                                    unsigned char gimbalDeviceId, float qw,
                                    float qx, float qy, float qz,
                                    float angularVelocityX, float angularVelocityY,
                                    float angularVelocityZ, float deltaYaw,
                                    float deltaYawVelocity);
    void roadsLookAt(const QGeoCoordinate& coordinate);
    void roadsStopLookAt();
    void setVNavEnabled(bool enabled);
    void setCurrentPosFromFlower(int lat, int lon);
    void setAHRSFromFlower(float roll, float pitch, float yaw, unsigned char gnss,
                           int lat, int lon, int alt, float heading,
                           float groundSpeed, float airSpeed);
    void setAHRSFromVNav(qlonglong time, int lat, int lon, unsigned int alt,
                         float gimbalRoll, float gimbalPitch, float gimbalYaw,
                         unsigned char status);
    void setAHRSByCamera(qlonglong time, float vehicleRoll, float vehiclePitch,
                         float vehicleYaw);
    void rpmValues(int rpm1, int rpm2);
    void vehicleDetected(qlonglong time, const QList<VehicleTarget>& targets);
    void setVehicleVersion(int value);
    void setAutoLandEnabled(bool enabled);
    void setInterceptorEnabled(bool enabled);
    void setExtremeGimbalTest(bool enabled);
    void setLoadImageTargetsFromClient(bool value);
    void setLoadImageMinelayerFromClient(bool value);
    void setLaunchReady(bool ready);
    void setBackupBattery(unsigned char id, short value);

private slots:
    void disableControlHandler();
    void setDisableVehicleMode();
    void setCircleMode();
    void setStabilizeMode();
    void rebootPTZCameraIfCalm();
    void rebootPTZCamera();
    void enablePTZCamera();
    void disablePTZCamera();
    void updatePreprocessingScale();
    void switchRoadDir();

private:
    EScout* scout_{};
    EPathfinderParams params_;
    PathfinderMode currentMode_{PathfinderMode::Disabled};
    LaunchMode launchMode_{LaunchMode::Mode1};
    int burnRocketDelayMs_{30};
    float airSpeedMetresPerSecond_{};
    float groundSpeedMetresPerSecond_{};
    Wind wind_{};
    Plan plan_;
    CameraParams cameraParams_;
    std::array<std::uint16_t, 14> rcValues_{};
    std::array<std::uint16_t, 14> copterRcValues_{};
    std::array<std::uint16_t, 3> controls_{};
    std::array<std::uint16_t, 3> copterControls_{};
    float manualMaximumAccumulation_{};
    qlonglong manualOldTimeOffset_{};
    qlonglong roadPartsTime_{};
    std::array<int, 3> roadParts_{};
    int dominantRoadPart_{3};
    int currentMissionIndex_{-1};
    int vehicleVersion_{};
    int batteryVoltage_{};
    bool ptzCamera_{};
    bool scoutValid_{};
    bool testPtzCenter_{};
    bool ptzCalibration_{};
    bool visionTest_{};
    bool objectDetectorEnabled_{};
    bool pathfinderVisionEnabled_{};
    bool videoSaverEnabled_{};
    bool roadsEnabled_{};
    bool released_{};
    bool vnavEnabled_{};
    bool autoLandEnabled_{};
    bool interceptorEnabled_{};
    bool extremeGimbalTestEnabled_{};
    bool loadImageTargetsFromClient_{};
    bool loadImageMinelayerFromClient_{};
    bool launchReady_{};
    bool seaMode_{};
    float preprocessingScale_{1.0F};
    PlaneTarget radarTarget_;
    QList<PlaneCoord> lidarTargets_;
    QList<VehicleTarget> vehicleTargets_;
    RoadValidateResult roadOffset_;
};
