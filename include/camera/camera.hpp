#pragma once

#include "camera/camera_types.hpp"

#include <QObject>
#include <QVector>

class Camera : public QObject {
    Q_OBJECT

public:
    explicit Camera(QObject* parent = nullptr);
    ~Camera() override = default;

    bool deviceIsEnabled() const;
    bool isConnected() const;
    bool isGimbalStabRoll() const;
    float calibrationWidthAngle() const;
    float calibrationHeightAngle() const;
    CameraParams currentParams() const;
    Gimbal gimbal(qlonglong time = 0) const;
    Zoom zoom(qlonglong time = 0) const;

    virtual void connect() = 0;
    virtual void disconnect() = 0;
    virtual void setZoom(float percent) = 0;
    virtual void setFocus(float distance, float zoomPercent, int timeoutMSec) = 0;

    void setGimbal(const Gimbal& gimbal);
    void setGimbalStabRoll(bool enabled);
    void setFrameBlur(float value);

signals:
    void connected();
    void disconnected();
    void deviceEnabled();
    void deviceDisabled();
    void paramsChanged(const CameraParams& params);
    void setPTZ(float roll,
                float pitch,
                float yaw,
                float rollMaxSpeed,
                float pitchMaxSpeed,
                float yawMaxSpeed,
                bool center,
                bool yawByNorth,
                unsigned char gimbalId);
    void ahrsByCamera(qlonglong time,
                      float vehicleRoll,
                      float vehiclePitch,
                      float vehicleYaw);

public slots:
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
                       float gimbalRoll,
                       float gimbalPitch,
                       float gimbalYaw);

protected:
    void setControlFeatures(bool zoomControlEnabled,
                            bool externalGimbalControl);
    void setDeviceEnabledState(bool enabled);
    void setConnectedState(bool connected);
    void updateZoom(float percent, qlonglong time = 0);
    void updateZoomFromOptical(float opticalZoom, qlonglong time);
    void applyZoomRecord(const Zoom& value, qlonglong time);
    void updateGimbalTelemetry(float roll,
                               float pitch,
                               float yaw,
                               float rollVelocity,
                               float pitchVelocity,
                               float yawVelocity,
                               qlonglong time);
    QVector<Zoom>& calibrationZooms();
    const QVector<Zoom>& calibrationZooms() const;

private:
    void calculateAnglesFromMag(Gimbal& value) const;
    void saveCurrentGimbal();
    void saveCurrentZoom();
    int previousIndex(int index) const;
    int nextIndex(int index) const;

    static constexpr int kHistorySize = 128;
    bool deviceEnabled_{};
    bool connected_{};
    bool zoomControlEnabled_{};
    bool externalGimbalControl_{};
    Gimbal currentGimbal_;
    float reservedA_{};
    float reservedB_{};
    float reservedC_{};
    int frameWidth_{1920};
    int frameHeight_{1080};
    Zoom currentZoom_{1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0};
    QVector<Zoom> calibrationZooms_;
    QVector<Gimbal> gimbalHistory_;
    QVector<Zoom> zoomHistory_;
    int gimbalIndex_{-1};
    int zoomIndex_{-1};
    bool gimbalHistoryInitialized_{};
    bool zoomHistoryInitialized_{};
    bool gimbalStabilizeRoll_{};
    unsigned char gimbalId_{1};
    float frameBlur_{100.0F};
};
