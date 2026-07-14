#pragma once

#include <QMetaType>
#include <QVector>

#include <cstdint>

// Binary stride 0x20. Angular fields are degrees. The final qword is a
// type-punned field in the recovered program: calibration records store two
// unit scale floats there, while live/history records store a millisecond
// timestamp. The union preserves both observed representations.
struct Zoom {
    struct CalibrationTail {
        float scaleX;
        float scaleY;
    };

    float opticalZoom{};
    float percent{};
    float widthAngle{};
    float heightAngle{};
    float scaleX{1.0F};
    float scaleY{1.0F};
    union {
        qlonglong time;
        CalibrationTail calibration;
    };

    Zoom() noexcept
        : time(0)
    {
    }

    Zoom(float optical,
         float percentValue,
         float horizontalAngle,
         float verticalAngle,
         float horizontalScale,
         float verticalScale,
         qlonglong timestamp = 0) noexcept
        : opticalZoom(optical)
        , percent(percentValue)
        , widthAngle(horizontalAngle)
        , heightAngle(verticalAngle)
        , scaleX(horizontalScale)
        , scaleY(verticalScale)
        , time(timestamp)
    {
    }
};

// Binary stride 0x58. The byte locations at 0x3f, 0x41, and 0x42 are
// directly consumed by Camera::setGimbal.
struct Gimbal {
    qlonglong time{};
    bool autoCenter{};
    std::uint8_t reserved09[3]{};
    float roll{};
    float yaw{};
    float pitch{};
    float rollMaxSpeed{};
    float pitchMaxSpeed{};
    float yawMaxSpeed{};
    float magRoll{};
    float magPitch{};
    float magYaw{};
    float vehicleRoll{};
    float vehiclePitch{};
    float vehicleYaw{};
    bool hasMagRoll{};
    bool hasMagPitch{};
    bool hasMagYaw{};
    bool center{};
    std::uint8_t reserved40{};
    std::uint8_t gimbalId{};
    bool yawByNorth{};
    std::uint8_t reserved43{};
    float angularVelocityRoll{};
    float angularVelocityPitch{};
    float angularVelocityYaw{};
    float deltaYaw{};
    float deltaYawVelocity{};
};

// Qt 5 layout is 0xa0 bytes. It embeds the current 0x58-byte gimbal at
// offset 8, the current 0x20-byte zoom at offset 0x78, and calibration zooms
// at offset 0x98.
struct CameraParams {
    bool zoomControlEnabled{};
    bool externalGimbalControl{};
    std::uint8_t reserved02[6]{};
    Gimbal gimbal;
    float reserved60{};
    float reserved64{};
    float reserved68{};
    int frameWidth{1920};
    int frameHeight{1080};
    std::uint32_t reserved74{};
    Zoom zoom;
    QVector<Zoom> calibrations;
};

static_assert(sizeof(Zoom) == 0x20U);
static_assert(sizeof(Gimbal) == 0x58U);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
static_assert(sizeof(CameraParams) == 0xa0U);
#endif

Q_DECLARE_METATYPE(Zoom)
Q_DECLARE_METATYPE(Gimbal)
Q_DECLARE_METATYPE(CameraParams)
