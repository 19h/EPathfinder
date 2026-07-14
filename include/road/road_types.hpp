#pragma once

#include <QGeoCoordinate>
#include <QList>
#include <QMap>
#include <QPointF>
#include <QVector>

#include <cstdint>

// Two-dimensional east/north displacement in metres.  The binary passes this
// value through the RoadMap and RoadRunner projection APIs as two binary64
// coordinates (RVA 0x78f50 and RVA 0x6b1f8).
struct RoadValidateOffset {
    double east{};
    double north{};
};

static_assert(sizeof(RoadValidateOffset) == 16U);

// QVector<RoadPoint> has a 0x20-byte stride in the original Qt 5 binary.
struct RoadPoint {
    std::int32_t number{-1};
    std::int32_t sourceIndex{-1};
    QGeoCoordinate coordinate;
    QPointF localCoordinate;
};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
static_assert(offsetof(RoadPoint, coordinate) == 8U);
static_assert(offsetof(RoadPoint, localCoordinate) == 16U);
static_assert(sizeof(RoadPoint) == 32U);
#endif

// Field positions are fixed by QVector<RoadItem>'s 0x38-byte stride and the
// constructors embedded in RoadMap::fromCoords (RVA 0x79cf0) and
// PlanHandler::roadMapFromPlan (RVA 0x192898).  Names describe observed use;
// four topology fields remain deliberately generic because their source names
// are absent from the ELF.
struct RoadItem {
    std::int32_t index{-1};
    std::int32_t planItemIndex{-1};
    std::int32_t point0{-1};
    std::int32_t point1{-1};
    std::int32_t topology0{-1};
    std::int32_t topology1{-1};
    std::int32_t topology2{-1};
    std::int32_t topology3{-1};
    float heading{};               // degrees clockwise from north
    std::int32_t topologyClass{4};
    std::int32_t direction{1};
    std::int32_t directionClass{4};
    float length{};                // metres
    std::int32_t reserved52{};
};

static_assert(offsetof(RoadItem, heading) == 32U);
static_assert(offsetof(RoadItem, length) == 48U);
static_assert(sizeof(RoadItem) == 56U);

// Heap payload used by QList<RoadElementDetection>; operator new(0x18) at RVA
// 0x75548 establishes the Qt 5 value size.
struct RoadElementDetection {
    qlonglong time{};
    QGeoCoordinate detectedCoordinate;
    QGeoCoordinate observerCoordinate;
};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
static_assert(sizeof(RoadElementDetection) == 24U);
#endif

// Exact 0x80-byte Qt 5 value layout recovered from the constructor/copy/
// assignment routines at RVAs 0x5d738, 0x5df30, and 0x5e060.  Fields whose
// semantic names cannot be proved are retained as source-erased state.
struct RoadElement {
    std::int32_t index{-1};
    std::int32_t type{5};
    QVector<float> headings;
    QVector<QVector<QPointF>> localItems;
    QVector<float> itemLengths;
    QGeoCoordinate coordinate;
    qlonglong state40{};
    qlonglong state48{};
    float score{};
    bool isFork{};
    bool isTurn{};
    bool enabled{true};
    std::uint8_t reserved63{};
    std::int32_t roadItem0{-1};
    std::int32_t roadItem1{-1};
    std::int32_t roadPoint0{-1};
    std::int32_t roadPoint1{-1};
    qlonglong firstDetectedTime{};
    qlonglong lastDetectedTime{};
    qlonglong state96{};
    QList<RoadElementDetection> detections;
    QList<float> detectionScores;
    float minimumConfidence{0.51F};
    std::uint32_t reserved124{};

    void addItem(float headingDegrees, const QVector<QPointF>& points);
};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
static_assert(offsetof(RoadElement, coordinate) == 32U);
static_assert(offsetof(RoadElement, isFork) == 60U);
static_assert(offsetof(RoadElement, lastDetectedTime) == 88U);
static_assert(offsetof(RoadElement, detections) == 104U);
static_assert(offsetof(RoadElement, minimumConfidence) == 120U);
static_assert(sizeof(RoadElement) == 128U);
#endif

struct RoadMap {
    QVector<RoadPoint> points;
    QVector<RoadItem> items;
    QVector<RoadElement> elements;
    QGeoCoordinate center;
    QMap<int, QVector<int>> outgoing;
    QMap<int, QVector<int>> incoming;

    QGeoCoordinate calcCenter();
    void toLocalCoord(const QGeoCoordinate& coordinate, QPointF& local) const;
    float mapNearestDistance(const QPointF& local,
                             QPointF& projected,
                             int& itemIndex) const;
    float projCoord(const QGeoCoordinate& coordinate,
                    QGeoCoordinate& projected,
                    RoadValidateOffset& offset,
                    int& itemIndex) const;
    bool checkNewCenter(const QGeoCoordinate& coordinate,
                        float thresholdMetres);
    void clear();
    void fromCoords(const QVector<QGeoCoordinate>& coordinates);
};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
static_assert(offsetof(RoadMap, center) == 24U);
static_assert(sizeof(RoadMap) == 48U);
#endif

Q_DECLARE_METATYPE(RoadElement)
Q_DECLARE_METATYPE(QVector<RoadElement>)

