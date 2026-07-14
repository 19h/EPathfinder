#pragma once

#include <QGeoCoordinate>
#include <QMetaType>

#include <cstdint>

// Exact 0x10-byte record appended by ThunderGaze::calculateRoadPointAngles at
// RVA 0x7e170. Angles are degrees relative to the optical axis.
struct RoadPointAngle {
    std::int16_t pixelX{};
    std::int16_t pixelY{};
    float angleX{};
    float angleY{};
    bool selected{};
    std::uint8_t reserved13[3]{};
};

static_assert(sizeof(RoadPointAngle) == 16U);

// Four binary64 image bounds.  QList<IgnoreBound>::append allocates 0x20
// bytes and copies four qwords at RVA 0x7e700.
struct IgnoreBound {
    double lowerX{};
    double lowerY{};
    double upperX{};
    double upperY{};
};

static_assert(sizeof(IgnoreBound) == 32U);

// Target::operator= at RVA 0x1323c0 proves the 0x58-byte Qt 5 layout.  The
// executable contains no source field names for the first 60 bytes, so their
// names are position-based rather than conjectural.  The scalar bit patterns
// are retained losslessly for every protocol generation.
struct Target {
    std::int32_t pixelX{};
    std::int32_t pixelY{};
    std::int32_t secondPixelX{};
    std::int32_t thirdPixelX{};
    std::int32_t auxiliaryPixelY{};
    std::int32_t lowerPixelY{};
    std::uint32_t state24{};
    float angleX{};
    float angleY{};
    float secondAngleX{};
    float secondAngleY{};
    float thirdAngleX{};
    float thirdAngleY{};
    float confidence{};
    std::int16_t state56{};
    std::int16_t type{};
    std::uint32_t reserved3C{};
    QGeoCoordinate coordinate;
    qlonglong time{};
    qlonglong state80{};
};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
static_assert(offsetof(Target, coordinate) == 64U);
static_assert(sizeof(Target) == 88U);
#endif

Q_DECLARE_METATYPE(RoadPointAngle)
Q_DECLARE_METATYPE(QVector<RoadPointAngle>)
Q_DECLARE_METATYPE(Target)
Q_DECLARE_METATYPE(QList<Target>)
