#pragma once

#include "scout/scout_types.hpp"

#include <QDateTime>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QString>
#include <QUdpSocket>
#include <QVector>

class EVision;
class RoadRunner;

class EScout : public QObject {
    Q_OBJECT

public:
    explicit EScout(QObject* parent = nullptr, bool isCopter = false);
    ~EScout() override = default;

    EPosition position(qulonglong time = 0);
    EPosition ePosition(qulonglong time = 0);
    EAttitude attitude(qulonglong time = 0, bool applyYawOffset = true);
    EAttitude eAttitude(qulonglong time = 0);
    EAttitude mainControllerAttitude(qulonglong time);

    ECoord ECoordOnLaunch() const;
    QGeoCoordinate QCoordOnLaunch() const;
    ECoord lastGpsECoord();
    QGeoCoordinate lastGpsCoord();

    int mslOnLaunch() const;
    int currentGroundMSL() const;
    int distanceSensorOffset() const;
    int currentState() const;
    qlonglong lastPositionTime() const;
    qlonglong lastAttitudeTime() const;
    qlonglong lastEPositionTime() const;
    qlonglong lastEAttitudeTime() const;
    qlonglong lastELinkEventTime() const;
    int satelliteCount() const;
    int eLinkSatelliteCount() const;
    int eLinkSatelliteStatus() const;
    int eLinkBackupSatelliteCount() const;
    unsigned int distSensorValue() const;
    qlonglong distSensorLastTime() const;
    qlonglong lastRecalcPositionsTime() const;
    GPSUsageMode currentGPSUsageMode() const;
    bool roadIsEnabled() const;
    bool correctRoadDistByGPSIsEnabled() const;
    bool isPTZDistSensor() const;
    bool lastRoadOffsetAccepted() const;
    qlonglong lastRoadOffsetTime() const;
    float lastRoadOffsetSize() const;
    qlonglong lastSavedCoordinateTime() const;
    float groundSpeed() const;
    float gpsGroundSpeed() const;
    qlonglong lastCalculatedWindTime() const;
    Wind lastCalculatedWind() const;
    Wind lastCalculatedAverageWind() const;
    float currentAhrsRPErrorAngle() const;
    float currentAhrsYawErrorAngle() const;
    qlonglong lastVNavCorrectTime() const;
    float lastVNavCorrectDistance() const;
    float lastVNavCorrectAzimuth() const;
    qlonglong lastVNav2FixTime() const;
    qlonglong lastVNav2TrackingTime() const;
    bool isExactMagValue() const;
    bool hasRoadOffsets() const;
    bool odoIsConnected() const;

    float azimuth();
    float gpsAzimuth();
    float currentVnavWindSpeed();
    qlonglong mav2SysTime(qlonglong time) const;
    qlonglong sys2MavTime(qlonglong time) const;

    void saveLastCoord(double latitude, double longitude);
    void setLaunchTime(qlonglong time);
    void setEDataEnabled(bool enabled);
    void setRawGpsAltitudeEnabled(bool enabled);
    void setWind(const Wind& wind);
    void setEIMUAirspeed(float airspeed);
    void setRoadRunner(RoadRunner* roadRunner);
    void addTargetAltOffset(int offset);
    void clearTargetAltOffset();
    void setElevationsPath(const QString& path);
    void setOdometry(int port, EVision* vision);
    void setLerpRollPitchSpeedCoeff(float coefficient);
    void setForceDisableGPS(bool disable);
    void setSatelliteCount(int count);
    void setDistanceSensorDivingEnabled(bool enabled);
    void setDistanceSensorUsageMode(DistanceSensorUsageMode mode);
    void setIsDiving(bool diving, qlonglong time);
    void setGPSUsageMode(GPSUsageMode mode);
    void setRoadsEnabled(bool enabled);
    void setCorrectRoadDistByGPSEnabled(bool enabled);
    void setPTZDistSensor(bool enabled);
    void setCorrectRoadByGPSMaxOffset(float offset);
    void setIsCopter(bool isCopter);

    void addMSL(double latitude, double longitude, double height);
    int coordMSL(const QGeoCoordinate& coordinate);
    void saveCoordOnLaunch(double latitude, double longitude, double msl);
    int elevationsCoordMLS(const QGeoCoordinate& coordinate);
    void calculateTemporaryOffset(const QGeoCoordinate& coordinate,
                                  QGeoCoordinate& output);

signals:
    void timeSynced();
    void takeoffed();
    void valid();
    void invalid();
    void addElinkTimeOffset(int value);
    void resetRoadMaxDistanceOffsetTime();
    void roadOffsetAccepted(const RoadValidateResult& result);
    void imuCorrectedByPeekAtGPS();
    void imuCorrected(float confidence,
                      float offset,
                      int objectIdx,
                      IMUCorrectSource source);

public slots:
    void setTimeOffset(qlonglong offset, int roundDt);
    void setCopterThrottle(int throttle);
    void setPosition(uint32_t timeBootMs,
                     int32_t lat,
                     int32_t lon,
                     int32_t alt,
                     int32_t mslAlt,
                     int16_t vx,
                     int16_t vy,
                     int16_t vz,
                     uint16_t hdg);
    void setRawPositionEvent(uint32_t timeBootMs,
                             int32_t lat,
                             int32_t lon,
                             int32_t alt);
    void setAttitude(uint32_t timeBootMs,
                     float pitch,
                     float yaw,
                     float roll,
                     float pitchSpeed,
                     float yawSpeed,
                     float rollSpeed);
    void setBackupAttitude(unsigned char linkId,
                           uint32_t timeBootMs,
                           float pitch,
                           float yaw,
                           float roll,
                           float pitchSpeed,
                           float yawSpeed,
                           float rollSpeed);
    void setAhrsError(float rpError, float yawError);
    void setDistanceSensor(unsigned int minDist,
                           unsigned int maxDist,
                           unsigned int curDist);
    void launchAcceleration();
    void setManuTakeoffed();
    void setSmoothAttSpeed(bool enabled);
    void landed();
    void setEPosition(qlonglong time,
                      unsigned char status,
                      unsigned char satellites,
                      int lat,
                      int lon,
                      int alt,
                      float vx,
                      float vy,
                      float vz,
                      float groundSpeed,
                      float groundCourse);
    void setEBackupPosition(qlonglong time,
                            unsigned char status,
                            unsigned char satellites,
                            int lat,
                            int lon,
                            int alt,
                            float vx,
                            float vy,
                            float vz,
                            float groundSpeed,
                            float groundCourse);
    void setEAttitude(qlonglong time, float pitch, float roll, float yaw);
    void setExactMagOffsetEnabled(bool enabled);
    void setExactMagTargetEnabled(bool enabled);
    void setCurrentExactMagValue(int value);
    void setExactMagDisableTime(qlonglong time);
    void setExactMagIsFreezed(bool freezed);
    void exactMagReseted();
    void setCopterAttitude(uint32_t timeBootMs,
                           float pitch,
                           float yaw,
                           float roll,
                           float pitchSpeed,
                           float yawSpeed,
                           float rollSpeed);
    void setCopterPosition(uint32_t timeBootMs,
                           int32_t lat,
                           int32_t lon,
                           int32_t alt,
                           int32_t mslAlt,
                           int16_t vx,
                           int16_t vy,
                           int16_t vz,
                           uint16_t hdg);
    void calibrateRollPitch();
    void calibrateYaw();
    void resetYawOffset();
    float currentYawOffset();
    void setELinkRCControlState(bool enabled);
    void setPTZDistSensorPitch(float pitch);
    void setDebugCorrectAltByPitch(bool enabled);
    void setRoadOffset(const RoadValidateResult& result);
    void setLerpDistSensorOffset(bool enabled);
    void selfDiagnostic();
    void setCurrentPosFromTelemetry(qlonglong time, int lat, int lon);
    void setCurrentPosFromVNav(qlonglong time,
                               int lat,
                               int lon,
                               float confidence);
    void setCurrentPosFromVNav(qlonglong time, int lat, int lon);
    void setCurrentPosFromImageTarget(qlonglong time, int lat, int lon);
    void setCurrentPosFromWayTargets(qlonglong time, int lat, int lon);
    void setCurrentPosFromFlower(qlonglong time, int lat, int lon);
    void setCurrentAHRSFromVNav(qlonglong time,
                                int lat,
                                int lon,
                                unsigned int alt,
                                float roll,
                                float pitch,
                                float yaw,
                                unsigned char status);
    void setYawOffsetFromVNav(qlonglong time, float offset);
    void setEnabledYawOffset(bool enabled);
    void undoLastRoadOffset();
    void setVNavIgnored(bool ignore);
    QGeoCoordinate lastRawIMU();
    void saveCoordinateByIMU();
    void restoreCoordinateByIMU();
    void setReplaceYawFromExactMag(bool enabled);
    void exactMagInited();
    void setInertCoeff(float coefficient);
    bool vnavAzimuth(float& azimuth);

private slots:
    void enableOdometry();
    void upConnectOdometry();
    void readOdoPendingDatagrams();
    void updateCurMSL();
    void autoUpdateExactMagOffset();
    void calcExactMagOffsetByMainMag();
    void readExactMagTable();

protected:
    void timerEvent(QTimerEvent* event) override;

private:
    template <typename T>
    int appendRing(QVector<T>& values, int& currentIndex, bool& initialized);
    template <typename T>
    int findItem(const QVector<T>& values,
                 int currentIndex,
                 qlonglong time) const;

    EPosition projectedPosition(const QVector<EPosition>& values,
                                int currentIndex,
                                qulonglong requestedTime,
                                bool eLink) const;
    EAttitude projectedAttitude(const QVector<EAttitude>& values,
                                int currentIndex,
                                qulonglong requestedTime,
                                bool applyYawOffset,
                                bool eLink);
    static float normalize360(float angle);
    static float signedAngularDelta(float from, float to);

    static constexpr int kHistorySize = 1024;
    QVector<EPosition> positions_;
    QVector<EAttitude> attitudes_;
    QVector<ERawGpsPosition> rawGpsPositions_;
    QVector<EPosition> rawImuPositions_;
    QVector<EPosition> imuPositions_;
    QVector<EPosition> ePositions_;
    QVector<EPosition> eBackupPositions_;
    QVector<EAttitude> eAttitudes_;
    QMap<unsigned char, EAttitude> backupAttitudes_;
    int positionIndex_{-1};
    int attitudeIndex_{-1};
    int rawGpsIndex_{-1};
    int ePositionIndex_{-1};
    int eBackupPositionIndex_{-1};
    int eAttitudeIndex_{-1};
    bool positionInitialized_{};
    bool attitudeInitialized_{};
    bool rawGpsInitialized_{};
    bool ePositionInitialized_{};
    bool eBackupPositionInitialized_{};
    bool eAttitudeInitialized_{};

    qlonglong timeOffset_{};
    int roundTrip_{};
    qlonglong launchTime_{};
    ECoord launchCoord_;
    QGeoCoordinate launchQCoord_;
    int launchMsl_{};
    int currentGroundMsl_{};
    int distanceSensorOffset_{};
    int targetAltitudeOffset_{};
    bool takeoffed_{};
    bool isCopter_{};
    int copterThrottle_{};

    int satelliteCount_{};
    int mainGpsState_{};
    int eLinkSatelliteCount_{};
    int eLinkSatelliteStatus_{};
    int eLinkBackupSatelliteCount_{};
    bool forceDisableGps_{};
    GPSUsageMode gpsUsageMode_{GPS_NOT_USE};
    DistanceSensorUsageMode distanceSensorUsageMode_{
        DISTANCE_SENSOR_NOT_USE};
    bool distanceSensorDivingEnabled_{};
    bool isDiving_{};
    qlonglong divingTime_{};
    unsigned int distanceSensorMin_{};
    unsigned int distanceSensorMax_{};
    unsigned int distanceSensorValue_{};
    qlonglong distanceSensorLastTime_{};
    qlonglong lastRecalcPositionsTime_{};

    bool eDataEnabled_{};
    bool rawGpsAltitudeEnabled_{};
    float eImuAirspeed_{};
    Wind wind_;
    float eGroundSpeed_{};
    qlonglong lastELinkEventTime_{};
    qlonglong lastCalculatedWindTime_{};
    Wind lastCalculatedWind_;
    Wind lastCalculatedAverageWind_;
    QList<Wind> realWinds_;

    float ahrsRpError_{};
    float ahrsYawError_{};
    qlonglong ahrsErrorTime_{};
    float calibrationPitchOffset_{};
    float calibrationRollOffset_{};
    float calibrationYawOffset_{};
    bool rollPitchCalibrated_{};
    bool yawCalibrated_{};
    bool smoothAttitudeSpeed_{};
    float lerpRollPitchSpeedCoeff_{0.8F};

    bool yawOffsetEnabled_{true};
    float yawOffset_{};
    qlonglong yawOffsetTime_{};
    int yawOffsetLifetimeMs_{7000};
    bool vnavIgnored_{};
    float vnavAzimuth_{-1.0F};
    float vnavWindSpeed_{};
    qlonglong vnavWindTime_{};
    qlonglong vnavWindLifetimeMs_{20000};
    qlonglong lastVNavCorrectTime_{};
    float lastVNavCorrectDistance_{};
    float lastVNavCorrectAzimuth_{};
    qlonglong lastVNav2FixTime_{};
    qlonglong lastVNav2TrackingTime_{};
    float inertCoeff_{0.8F};

    bool roadsEnabled_{true};
    bool correctRoadDistanceByGpsEnabled_{};
    float correctRoadByGpsMaxOffset_{};
    RoadValidateResult roadOffset_;
    RoadValidateResult previousRoadOffset_;
    bool hasRoadOffset_{};
    bool ptzDistanceSensor_{};
    float ptzDistanceSensorPitch_{};
    bool debugCorrectAltitudeByPitch_{};
    bool lerpDistanceSensorOffset_{};
    bool eLinkRcControlState_{};
    RoadRunner* roadRunner_{};

    QString elevationsPath_;
    QVector<QPair<QGeoCoordinate, double>> elevations_;
    QMap<QPair<int, int>, double> mslMap_;

    bool exactMagOffsetEnabled_{};
    bool exactMagTargetEnabled_{};
    bool exactMagFreezed_{};
    bool exactMagInitialized_{};
    bool exactMagValueValid_{};
    bool replaceYawFromExactMag_{};
    int exactMagValue_{};
    qlonglong exactMagDisableTime_{};
    QList<float> exactMagOffsets_;

    QUdpSocket* odometrySocket_{};
    int odometryPort_{};
    EVision* vision_{};
    bool odometryEnabled_{};
    QGeoCoordinate savedCoordinateByImu_;
    bool hasSavedCoordinateByImu_{};
    qlonglong lastSavedCoordinateTime_{};
};
