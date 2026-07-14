#pragma once

#include "road/road_types.hpp"

#include <QGeoCoordinate>
#include <QObject>
#include <QPointF>
#include <QSet>
#include <QVector>

class RoadAnalyzer : public QObject {
    Q_OBJECT

public:
    explicit RoadAnalyzer(QObject* parent = nullptr);
    ~RoadAnalyzer() override = default;

    void setMap(RoadMap* map);
    virtual void analyzeMap() = 0;
    virtual void searchElements(const QGeoCoordinate& planeCoordinate,
                                const QPointF& planeLocal,
                                const QGeoCoordinate& viewCenter,
                                const QVector<QPointF>& visiblePoints,
                                int previousPlanIndex,
                                int nextPlanIndex,
                                float planeYawDegrees,
                                float viewAngleDegrees,
                                bool exactMagneticValue) = 0;

signals:
    void elementsDetected(const QVector<RoadElement>& elements);

protected:
    RoadMap* map_{};
};

class RoadAnalyzerV1 final : public RoadAnalyzer {
    Q_OBJECT

public:
    explicit RoadAnalyzerV1(QObject* parent = nullptr);
    void analyzeMap() override;
    void searchElements(const QGeoCoordinate& planeCoordinate,
                        const QPointF& planeLocal,
                        const QGeoCoordinate& viewCenter,
                        const QVector<QPointF>& visiblePoints,
                        int previousPlanIndex,
                        int nextPlanIndex,
                        float planeYawDegrees,
                        float viewAngleDegrees,
                        bool exactMagneticValue) override;
};

class RoadAnalyzerV2 : public RoadAnalyzer {
    Q_OBJECT

public:
    explicit RoadAnalyzerV2(QObject* parent = nullptr);
    void analyzeMap() override;
    void searchElements(const QGeoCoordinate& planeCoordinate,
                        const QPointF& planeLocal,
                        const QGeoCoordinate& viewCenter,
                        const QVector<QPointF>& visiblePoints,
                        int previousPlanIndex,
                        int nextPlanIndex,
                        float planeYawDegrees,
                        float viewAngleDegrees,
                        bool exactMagneticValue) override;
    QVector<QPointF>& localPoints();
    const QVector<QPointF>& localPoints() const;

protected:
    QVector<QPointF> localPoints_;
};

class RoadNetworkAnalyzer final : public RoadAnalyzerV2 {
    Q_OBJECT

public:
    explicit RoadNetworkAnalyzer(QObject* parent = nullptr);
    void analyzeMap() override;
    void searchElements(const QGeoCoordinate& planeCoordinate,
                        const QPointF& planeLocal,
                        const QGeoCoordinate& viewCenter,
                        const QVector<QPointF>& visiblePoints,
                        int previousPlanIndex,
                        int nextPlanIndex,
                        float planeYawDegrees,
                        float viewAngleDegrees,
                        bool exactMagneticValue) override;

    QVector<float> filteredOffsets(float maximumAbsoluteOffset) const;
    bool isUniqueDotPattern(const QVector<QPointF>& points) const;
    float dispersion(const QVector<QPointF>& points,
                     const QPointF& center,
                     QSet<int>& selected,
                     float radius) const;

private:
    QVector<float> offsets_;
};

