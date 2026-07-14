#pragma once

#include "camera/camera.hpp"
#include "vision/thunder_gaze.hpp"

#include <QByteArray>
#include <QJsonDocument>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

class QUdpSocket;

class EVision : public QObject {
    Q_OBJECT

public:
    explicit EVision(int port,
                     float thunderGazeVersion,
                     QObject* parent = nullptr);
    ~EVision() override = default;

    bool isCalibrated() const;
    bool isConnected() const;
    void setCameraOffset(float horizontalDegrees, float verticalDegrees);
    void setPTZCalibration(float distanceMetres,
                           float horizontalOffsetMillimetres,
                           float verticalOffsetMillimetres);
    float cameraCalibrationWidthAngle() const;
    float cameraCalibrationHeightAngle() const;
    void setUSBCamera(const QString& device);
    void setIPCamera(const QString& address,
                     int localPort,
                     int remotePort,
                     bool externalGimbalControl);
    bool cameraIsEnabled() const;
    void setGimbalStabRoll(bool enabled);
    void pointAngles(int pixelX,
                     int pixelY,
                     float& horizontalDegrees,
                     float& verticalDegrees) const;
    void setDebugDTPEnabled(bool enabled);
    void setDetTypes(const QVector<unsigned short>& types);
    void setRoadsEnabled(bool enabled);
    void setRoadReduceBorderEnabled(bool enabled);
    void setIsRailway(unsigned char value);
    void setTelemetryParams(const QJsonDocument& params);
    void setTestTarget(int pixelX, int pixelY);
    Camera* camera() const;
    ThunderGaze* thunderGaze() const;

signals:
    void targetDetected(const CameraParams& params,
                        const QList<Target>& targets);
    void imageTargetDetected(const CameraParams& params,
                             const Target& target);
    void roadDetected(qlonglong time,
                      const CameraParams& cameraParams,
                      const QVector<RoadPointAngle>& roadPoints,
                      bool isSingleLine);
    void roadScreenParts(qlonglong time, int left, int middle, int right);
    void emptyDTP();
    void cameraParamsChanged(const CameraParams& params);
    void setPTZ(float roll,
                float pitch,
                float yaw,
                float rollMaxSpeed,
                float pitchMaxSpeed,
                float yawMaxSpeed,
                bool center,
                bool yawByNorth,
                unsigned char gimbalId);
    void preprocessingScaleChanged(float value);
    void rtspEnabled();
    void rtspDisabled();
    void dtpDebugInfo(const QString& text);
    void objectDetectorEnabled(bool enabled);
    void pathfinderEnabled(bool enabled);
    void videoSaverEnabled(bool enabled);
    void ahrsByCamera(qlonglong time,
                      float vehicleRoll,
                      float vehiclePitch,
                      float vehicleYaw);

public slots:
    void connectToCamera();
    void setGimbal(const Gimbal& gimbal);
    void gimbalValues(float roll,
                      float pitch,
                      float yaw,
                      bool isCenter,
                      qlonglong time);
    void gimbalRollMagValue(qlonglong time,
                            unsigned char componentId,
                            float roll,
                            float pitch,
                            float yaw,
                            float magRoll);
    void gimbalPitchMagValue(qlonglong time,
                             unsigned char componentId,
                             float roll,
                             float pitch,
                             float yaw,
                             float magPitch);
    void gimbalYawMagValue(qlonglong time,
                           unsigned char componentId,
                           float roll,
                           float pitch,
                           float yaw,
                           float magYaw);
    void gimbalMagValues(qlonglong time,
                         unsigned char componentId,
                         float roll,
                         float pitch,
                         float yaw,
                         float magRoll,
                         float magPitch,
                         float magYaw);
    void setGimbalAHRS(qlonglong time,
                       int latitude,
                       int longitude,
                       unsigned int altitude,
                       float gimbalRoll,
                       float gimbalPitch,
                       float gimbalYaw,
                       unsigned char status);
    void setZoom(float percent);
    void setFocus(float distance, float zoomPercent, int timeoutMSec);
    void setPreprocessingScale(float value);
    void setRTSPEnabled(bool enabled);
    void enableRTSP();
    void disableRTSP();
    void setRoadSimpleMode(bool enabled);
    void setFrameText(const QStringList& lines);
    void setObjectDetectorEnabled(bool enabled);
    void setPathfinderEnabled(bool enabled);
    void setVideoSaverEnabled(bool enabled);
    void sendImageTarget(qlonglong time,
                         short pixelX,
                         short pixelY,
                         unsigned char type);

private slots:
    void readPendingDatagrams();
    void upConnect();
    void setTargets(QList<Target>& targets);
    void frameBlur(float value);

private:
    struct CalibrationPoint {
        double pixelX{};
        double pixelY{};
        float horizontalDegrees{};
        float verticalDegrees{};
        bool valid{};
    };

    void initConnect();
    void loadCalibration();
    void connectCameraSignals();
    void setTargetAngles(Target& target,
                         const CameraParams& params) const;
    void targetAnglesForPixel(int pixelX,
                              int pixelY,
                              const CameraParams& params,
                              float& horizontalDegrees,
                              float& verticalDegrees) const;

    bool calibrated_{};
    int port_{};
    QUdpSocket* socket_{};
    QByteArray receiveBuffer_;
    int calibrationStepX_{1};
    int calibrationStepY_{1};
    float cameraOffsetX_{};
    float cameraOffsetY_{};
    float ptzCalibrationX_{};
    float ptzCalibrationY_{};
    QVector<CalibrationPoint> calibrationPoints_;
    Camera* camera_{};
    ThunderGaze* thunderGaze_{};
    float thunderGazeVersion_{};
};

