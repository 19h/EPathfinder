#pragma once

#include <QGeoCoordinate>
#include <QMetaType>
#include <QVector>

#include <cstdint>

enum GPSUsageMode : int {
    GPS_NOT_USE = 0,
    GPS_ACTIVE_FIND = 1,
    GPS_FORCE_USE = 2,
};

enum DistanceSensorUsageMode : int {
    DISTANCE_SENSOR_NOT_USE = 0,
    DISTANCE_SENSOR_NORMAL_ANGLES = 1,
    DISTANCE_SENSOR_EXTERNAL_ANGLES = 2,
};

enum IMUCorrectSource : int {
    IMU_CORRECT_AHRS = 0,
    IMU_CORRECT_VNAV = 1,
    IMU_CORRECT_ROAD = 2,
    IMU_CORRECT_TELEMETRY = 3,
    IMU_CORRECT_FLOWER = 4,
    IMU_CORRECT_IMAGE_TARGET = 5,
    IMU_CORRECT_WAY_TARGETS = 6,
};

struct Wind {
    float direction{};
    float speed{};
};

// Binary stride 0x20. This is the prefix copied by lastGpsECoord and the
// launch-coordinate accessor.
struct ECoord {
    int type{};
    int lat{};       // 10^-7 degree
    int lon{};       // 10^-7 degree
    int alt{};       // mm
    int mslAlt{};    // mm
    bool valid{true};
    std::uint8_t reserved0[3]{};
    float groundSpeed{};
    float groundCourse{};
};

// Binary stride 0x58. The second coordinate block is retained because the
// original alternates GPS and inertially propagated representations in the
// same ring item.
struct EPosition : ECoord {
    int secondaryType{};
    int secondaryLat{};
    int secondaryLon{};
    int secondaryAlt{};
    int secondaryMslAlt{};
    bool secondaryValid{true};
    std::uint8_t reserved1[3]{};
    std::uint64_t reserved2{};
    std::int16_t vx{}; // cm/s, north
    std::int16_t vy{}; // cm/s, east
    std::int16_t vz{}; // cm/s, down
    std::uint16_t reserved3{};
    qlonglong time{};  // ms, system epoch
    float azimuth{};   // degree
    std::uint32_t reserved4{};
};

// Binary stride 0x30. Angular rates use degree/ms.
struct EAttitude {
    float pitch{};
    float yaw{};
    float roll{};
    float pitchSpeed{};
    float yawSpeed{};
    float rollSpeed{};
    float futurePitchSpeed{};
    float futureRollSpeed{};
    float futureYaw{};
    std::uint32_t reserved{};
    qlonglong time{}; // ms, system epoch
};

struct ERawGpsPosition {
    int type{1};
    int lat{};
    int lon{};
    int alt{};
    int mslAlt{};
    bool valid{true};
    std::uint8_t reserved0[3]{};
    float groundSpeed{};
    float groundCourse{};
    qlonglong time{};
};

// Source-visible aggregate reconstructed from its 0x58-byte Qt 5 value
// layout. Descriptive field names replace erased private identifiers.
struct RoadValidateResult {
    qlonglong time{};
    int objectIdx{};
    float confidence{};
    float offset{};
    int roadItem{-1};
    bool accepted{};
    QVector<int> roadItems;
    qlonglong sourceTime{};
    float offsetX{};
    float offsetY{};
    qlonglong reserved0{};
    qlonglong reserved1{};
    bool temporary{};
    QGeoCoordinate coordinate;
};

Q_DECLARE_METATYPE(GPSUsageMode)
Q_DECLARE_METATYPE(DistanceSensorUsageMode)
Q_DECLARE_METATYPE(IMUCorrectSource)
Q_DECLARE_METATYPE(Wind)
Q_DECLARE_METATYPE(ECoord)
Q_DECLARE_METATYPE(EPosition)
Q_DECLARE_METATYPE(EAttitude)
Q_DECLARE_METATYPE(RoadValidateResult)
