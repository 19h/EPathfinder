#include "road/roads.hpp"

#include <QLineF>
#include <QPolygonF>

#include <algorithm>
#include <cmath>

namespace {

float circularDifference(float first, float second)
{
    float difference = std::fmod(std::abs(first - second), 360.0F);
    if (difference > 180.0F) {
        difference = 360.0F - difference;
    }
    return difference;
}

QVector<RoadElement> buildElements(RoadMap& map, const bool includeTurns)
{
    QVector<RoadElement> elements;
    for (int pointIndex = 0; pointIndex < map.points.size(); ++pointIndex) {
        QVector<int> incident;
        for (int itemIndex = 0; itemIndex < map.items.size(); ++itemIndex) {
            const RoadItem& item = map.items.at(itemIndex);
            if (item.point0 == pointIndex || item.point1 == pointIndex) {
                incident.append(itemIndex);
            }
        }
        bool isTurn = false;
        if (incident.size() == 2) {
            const float first = map.items.at(incident.at(0)).heading;
            const float second = map.items.at(incident.at(1)).heading;
            const float difference = circularDifference(first, second);
            isTurn = difference > 20.0F && difference < 160.0F;
        }
        if (incident.size() == 2 && (!includeTurns || !isTurn)) {
            continue;
        }
        if (incident.isEmpty()) {
            continue;
        }

        RoadElement element;
        element.index = elements.size();
        element.coordinate = map.points.at(pointIndex).coordinate;
        element.roadPoint0 = pointIndex;
        element.isFork = incident.size() > 2;
        element.isTurn = isTurn;
        for (const int itemIndex : incident) {
            const RoadItem& item = map.items.at(itemIndex);
            const int other = item.point0 == pointIndex
                ? item.point1
                : item.point0;
            if (other < 0 || other >= map.points.size()) {
                continue;
            }
            const QPointF relative = map.points.at(other).localCoordinate
                - map.points.at(pointIndex).localCoordinate;
            const float heading = static_cast<float>(
                std::atan2(relative.x(), relative.y())
                * 57.29577951308232);
            element.addItem(heading, {relative});
        }
        if (!incident.isEmpty()) {
            element.roadItem0 = incident.first();
            element.roadItem1 = incident.last();
        }
        elements.append(element);
    }
    return elements;
}

QVector<RoadElement> visibleElements(RoadMap* const map,
                                     const QVector<QPointF>& visiblePoints)
{
    if (map == nullptr) {
        return {};
    }
    if (visiblePoints.size() < 3) {
        return map->elements;
    }
    const QPolygonF polygon(visiblePoints);
    QVector<RoadElement> selected;
    for (const RoadElement& element : map->elements) {
        QPointF local;
        map->toLocalCoord(element.coordinate, local);
        if (polygon.containsPoint(local, Qt::OddEvenFill)) {
            selected.append(element);
        }
    }
    return selected;
}

} // namespace

RoadAnalyzer::RoadAnalyzer(QObject* const parent)
    : QObject(parent)
{
}

void RoadAnalyzer::setMap(RoadMap* const map) { map_ = map; }

RoadAnalyzerV1::RoadAnalyzerV1(QObject* const parent)
    : RoadAnalyzer(parent)
{
}

void RoadAnalyzerV1::analyzeMap()
{
    if (map_ == nullptr) {
        emit elementsDetected({});
        return;
    }
    map_->elements = buildElements(*map_, false);
    emit elementsDetected(map_->elements);
}

void RoadAnalyzerV1::searchElements(const QGeoCoordinate&,
                                    const QPointF&,
                                    const QGeoCoordinate&,
                                    const QVector<QPointF>& visiblePoints,
                                    int,
                                    int,
                                    float,
                                    float,
                                    bool)
{
    emit elementsDetected(visibleElements(map_, visiblePoints));
}

RoadAnalyzerV2::RoadAnalyzerV2(QObject* const parent)
    : RoadAnalyzer(parent)
{
}

void RoadAnalyzerV2::analyzeMap()
{
    if (map_ == nullptr) {
        localPoints_.clear();
        emit elementsDetected({});
        return;
    }
    localPoints_.clear();
    localPoints_.reserve(map_->points.size());
    for (const RoadPoint& point : map_->points) {
        localPoints_.append(point.localCoordinate);
    }
    map_->elements = buildElements(*map_, true);
    emit elementsDetected(map_->elements);
}

void RoadAnalyzerV2::searchElements(const QGeoCoordinate&,
                                    const QPointF&,
                                    const QGeoCoordinate&,
                                    const QVector<QPointF>& visiblePoints,
                                    int,
                                    int,
                                    float,
                                    float,
                                    bool)
{
    emit elementsDetected(visibleElements(map_, visiblePoints));
}

QVector<QPointF>& RoadAnalyzerV2::localPoints() { return localPoints_; }

const QVector<QPointF>& RoadAnalyzerV2::localPoints() const
{
    return localPoints_;
}

RoadNetworkAnalyzer::RoadNetworkAnalyzer(QObject* const parent)
    : RoadAnalyzerV2(parent)
{
}

void RoadNetworkAnalyzer::analyzeMap() { RoadAnalyzerV2::analyzeMap(); }

void RoadNetworkAnalyzer::searchElements(
    const QGeoCoordinate& planeCoordinate,
    const QPointF& planeLocal,
    const QGeoCoordinate& viewCenter,
    const QVector<QPointF>& visiblePoints,
    const int previousPlanIndex,
    const int nextPlanIndex,
    const float planeYawDegrees,
    const float viewAngleDegrees,
    const bool exactMagneticValue)
{
    offsets_.clear();
    for (const QPointF& point : visiblePoints) {
        offsets_.append(static_cast<float>(QLineF(planeLocal, point).length()));
    }
    RoadAnalyzerV2::searchElements(planeCoordinate,
                                   planeLocal,
                                   viewCenter,
                                   visiblePoints,
                                   previousPlanIndex,
                                   nextPlanIndex,
                                   planeYawDegrees,
                                   viewAngleDegrees,
                                   exactMagneticValue);
}

QVector<float> RoadNetworkAnalyzer::filteredOffsets(
    const float maximumAbsoluteOffset) const
{
    QVector<float> result;
    for (const float offset : offsets_) {
        if (std::abs(offset) <= maximumAbsoluteOffset) {
            result.append(offset);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

bool RoadNetworkAnalyzer::isUniqueDotPattern(
    const QVector<QPointF>& points) const
{
    QSet<QPair<qint64, qint64>> quantized;
    for (const QPointF& point : points) {
        const QPair<qint64, qint64> key(
            std::llround(point.x() * 1000.0),
            std::llround(point.y() * 1000.0));
        if (quantized.contains(key)) {
            return false;
        }
        quantized.insert(key);
    }
    return true;
}

float RoadNetworkAnalyzer::dispersion(const QVector<QPointF>& points,
                                      const QPointF& center,
                                      QSet<int>& selected,
                                      const float radius) const
{
    selected.clear();
    if (points.isEmpty()) {
        return 0.0F;
    }
    double sumSquared = 0.0;
    for (int index = 0; index < points.size(); ++index) {
        const double distance = QLineF(center, points.at(index)).length();
        if (distance <= radius) {
            selected.insert(index);
            sumSquared += distance * distance;
        }
    }
    return selected.isEmpty()
        ? 0.0F
        : static_cast<float>(
            std::sqrt(sumSquared / static_cast<double>(selected.size())));
}

