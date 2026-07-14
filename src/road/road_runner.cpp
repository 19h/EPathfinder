#include "road/road_runner.hpp"

#include <QDateTime>
#include <QLineF>
#include <QTimerEvent>

#include <algorithm>
#include <cmath>
#include <limits>

RoadRunner::RoadRunner(QObject* const parent)
    : QObject(parent)
    , analyzer_(new RoadAnalyzerV2(this))
{
    qRegisterMetaType<RoadValidateResult>("RoadValidateResult");
    qRegisterMetaType<QVector<RoadElement>>("QVector<RoadElement>");
    connect(analyzer_,
            &RoadAnalyzer::elementsDetected,
            this,
            &RoadRunner::detectedElementsHandler);
    matchTimerId_ = startTimer(1400, Qt::CoarseTimer);
}

void RoadRunner::setMaxYawOffset(const float degrees)
{
    maximumYawOffsetDegrees_ = degrees;
}

void RoadRunner::setMinConfidence(const float value)
{
    minimumConfidence_ = value;
}

bool RoadRunner::isEnabled() const { return enabled_; }

bool RoadRunner::isCalculatePlaneYawEnabled() const
{
    return calculatePlaneYawEnabled_;
}

void RoadRunner::setCalculatePlaneYawEnabled(const bool enabled)
{
    calculatePlaneYawEnabled_ = enabled;
}

bool RoadRunner::isSearchRoadElementsEnabled() const
{
    return searchRoadElementsEnabled_;
}

void RoadRunner::setSearchRoadElementsEnabled(const bool enabled)
{
    searchRoadElementsEnabled_ = enabled;
}

bool RoadRunner::isAlongOffsetEnabled() const
{
    return alongOffsetEnabled_;
}

void RoadRunner::setAlongOffsetEnabled(const bool enabled)
{
    alongOffsetEnabled_ = enabled;
}

bool RoadRunner::calculateTemporaryOffset(
    const QGeoCoordinate& coordinate,
    QGeoCoordinate& shifted) const
{
    if (!enabled_ || !currentOffset_.accepted) {
        shifted = coordinate;
        return false;
    }
    shifted = coordinate.atDistanceAndAzimuth(currentOffset_.offsetX, 90.0);
    shifted = shifted.atDistanceAndAzimuth(currentOffset_.offsetY, 0.0);
    return true;
}

qlonglong RoadRunner::lastDetectedTimeRoadItem(
    const int planItemIndex) const
{
    return enabled_ ? roadItemDetectionTimes_.value(planItemIndex, 0) : 0;
}

bool RoadRunner::hasTurns() const
{
    if (map_ == nullptr) {
        return false;
    }
    return std::any_of(map_->elements.cbegin(),
                       map_->elements.cend(),
                       [](const RoadElement& element) {
                           return element.isTurn;
                       });
}

void RoadRunner::setMode(const Mode mode) { mode_ = mode; }
RoadRunner::Mode RoadRunner::currentMode() const { return mode_; }

void RoadRunner::movePlaneAlongRoad(const QGeoCoordinate&,
                                    int,
                                    float,
                                    float& eastMetres,
                                    float& northMetres) const
{
    // The recovered binary implementation at RVA 0x68db0 is an explicit stub.
    eastMetres = 0.0F;
    northMetres = 0.0F;
}

bool RoadRunner::isEqualOffsets(const double firstEast,
                                const double firstNorth,
                                const double secondEast,
                                const double secondNorth) const
{
    return std::hypot(secondEast - firstEast,
                      secondNorth - firstNorth)
        < 200.0;
}

void RoadRunner::accept()
{
    if (!pendingOffsets_.isEmpty()) {
        currentOffset_ = pendingOffsets_.last();
        currentOffset_.accepted = true;
        acceptedOffsets_.append(currentOffset_);
        emit offsetChanged(currentOffset_);
    }
    pendingOffsets_.clear();
}

void RoadRunner::match() { calculateAverageOffset(); }

QList<RoadElement*> RoadRunner::planPointRoadElements() const
{
    QList<RoadElement*> output;
    if (map_ == nullptr) {
        return output;
    }
    for (RoadElement& element : map_->elements) {
        if (element.roadPoint0 >= 0) {
            output.append(&element);
        }
    }
    return output;
}

void RoadRunner::clear()
{
    pendingOffsets_.clear();
    detectedElements_.clear();
    acceptedOffsets_.clear();
}

void RoadRunner::setEnabled(const bool enabled)
{
    enabled_ = enabled;
    if (!enabled_) {
        clear();
    }
}

qlonglong RoadRunner::lastDetectedTimeElement(const int index) const
{
    if (!enabled_ || map_ == nullptr || index < 0
        || index >= map_->elements.size()) {
        return 0;
    }
    return map_->elements.at(index).lastDetectedTime;
}

QGeoCoordinate RoadRunner::lastTurn() const
{
    if (map_ == nullptr) {
        return {};
    }
    for (auto iterator = map_->elements.crbegin();
         iterator != map_->elements.crend();
         ++iterator) {
        if (iterator->isTurn) {
            return iterator->coordinate;
        }
    }
    return {};
}

bool RoadRunner::roadElementIsFork(const int index) const
{
    return map_ != nullptr && index >= 0 && index < map_->elements.size()
        && map_->elements.at(index).isFork;
}

void RoadRunner::offsetByAlongs()
{
    if (alongOffsetEnabled_) {
        calculateAverageOffset();
    }
}

void RoadRunner::calcNearbyMapItems(
    const QGeoCoordinate& coordinate,
    const float minimumDistanceMetres,
    const float maximumDistanceMetres,
    const float minimumHeadingDegrees,
    const float maximumHeadingDegrees,
    QList<int>& output) const
{
    output.clear();
    if (map_ == nullptr) {
        return;
    }
    QPointF local;
    map_->toLocalCoord(coordinate, local);
    for (const RoadItem& item : map_->items) {
        if (item.point0 < 0 || item.point0 >= map_->points.size()
            || item.point1 < 0 || item.point1 >= map_->points.size()) {
            continue;
        }
        const QLineF line(map_->points.at(item.point0).localCoordinate,
                          map_->points.at(item.point1).localCoordinate);
        const float distance = static_cast<float>(
            std::min(QLineF(local, line.p1()).length(),
                     QLineF(local, line.p2()).length()));
        if (distance >= minimumDistanceMetres
            && distance <= maximumDistanceMetres
            && headingInRange(item.heading,
                              minimumHeadingDegrees,
                              maximumHeadingDegrees)) {
            output.append(item.index);
        }
    }
}

float RoadRunner::mapNearestDistance(const QPointF& local,
                                     QPointF& projected,
                                     int& itemIndex) const
{
    if (map_ == nullptr) {
        itemIndex = -1;
        return std::numeric_limits<float>::max();
    }
    return map_->mapNearestDistance(local, projected, itemIndex);
}

float RoadRunner::mapNearestDistance(const QGeoCoordinate& coordinate,
                                     QPointF& projected,
                                     int& itemIndex) const
{
    if (map_ == nullptr) {
        itemIndex = -1;
        return std::numeric_limits<float>::max();
    }
    QPointF local;
    map_->toLocalCoord(coordinate, local);
    return map_->mapNearestDistance(local, projected, itemIndex);
}

float RoadRunner::roadProjCoord(const QGeoCoordinate& coordinate,
                                QGeoCoordinate& projected,
                                RoadValidateOffset& offset,
                                int& itemIndex) const
{
    if (map_ == nullptr) {
        itemIndex = -1;
        offset = {};
        projected = coordinate;
        return std::numeric_limits<float>::max();
    }
    return map_->projCoord(coordinate, projected, offset, itemIndex);
}

bool RoadRunner::pathsIsEqual(
    const QVector<QPair<QGeoCoordinate, QGeoCoordinate>>& paths) const
{
    if (map_ == nullptr || paths.size() != map_->items.size()) {
        return false;
    }
    for (const auto& path : paths) {
        bool found = false;
        for (const RoadItem& item : map_->items) {
            if (item.point0 < 0 || item.point0 >= map_->points.size()
                || item.point1 < 0 || item.point1 >= map_->points.size()) {
                continue;
            }
            const QGeoCoordinate& first =
                map_->points.at(item.point0).coordinate;
            const QGeoCoordinate& second =
                map_->points.at(item.point1).coordinate;
            const bool direct = first.distanceTo(path.first) < 1.0
                && second.distanceTo(path.second) < 1.0;
            const bool reverse = first.distanceTo(path.second) < 1.0
                && second.distanceTo(path.first) < 1.0;
            if (direct || reverse) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

float RoadRunner::roadDistance(const QGeoCoordinate& coordinate) const
{
    QPointF projected;
    int itemIndex = -1;
    return mapNearestDistance(coordinate, projected, itemIndex);
}

void RoadRunner::calculateLocalCoords()
{
    if (map_ == nullptr) {
        return;
    }
    if (centerPosition_.isValid()) {
        map_->center = centerPosition_;
    } else if (!map_->center.isValid()) {
        map_->calcCenter();
    }
    for (RoadPoint& point : map_->points) {
        map_->toLocalCoord(point.coordinate, point.localCoordinate);
    }
    for (RoadItem& item : map_->items) {
        if (item.point0 < 0 || item.point0 >= map_->points.size()
            || item.point1 < 0 || item.point1 >= map_->points.size()) {
            continue;
        }
        const QPointF delta = map_->points.at(item.point1).localCoordinate
            - map_->points.at(item.point0).localCoordinate;
        item.heading = static_cast<float>(
            std::atan2(delta.x(), delta.y()) * 57.29577951308232);
        if (item.heading < 0.0F) {
            item.heading += 360.0F;
        }
        item.length = static_cast<float>(
            map_->points.at(item.point0).coordinate.distanceTo(
                map_->points.at(item.point1).coordinate));
    }
}

void RoadRunner::setMap(RoadMap* const map)
{
    map_ = map;
    if (map_ == nullptr) {
        analyzer_->setMap(nullptr);
        return;
    }
    map_->calcCenter();
    calculateLocalCoords();
    analyzer_->setMap(map_);
    analyzer_->analyzeMap();
}

void RoadRunner::setCenterPos(const QGeoCoordinate& coordinate)
{
    centerPosition_ = coordinate;
    calculateLocalCoords();
}

void RoadRunner::addData(
    const QVector<QGeoCoordinate>& visibleCoords,
    const bool isSingleRoadLine,
    const QGeoCoordinate& planeCoord,
    const QGeoCoordinate& planeRawCoord,
    const QGeoCoordinate& viewCenter,
    const int prevWPPlanIdx,
    const int nextWPPlanIdx,
    const QGeoCoordinate& prevWPCoord,
    const QGeoCoordinate& nextWPCoord,
    const int planeAltitude,
    const float planeYaw,
    const float viewAngle,
    const float viewDist,
    const float currentYawOffset,
    const float maxOffset,
    const bool isExactMagValue)
{
    addData2(visibleCoords,
             isSingleRoadLine,
             planeCoord,
             planeRawCoord,
             viewCenter,
             prevWPPlanIdx,
             nextWPPlanIdx,
             prevWPCoord,
             nextWPCoord,
             planeAltitude,
             planeYaw,
             viewAngle,
             viewDist,
             currentYawOffset,
             maxOffset,
             isExactMagValue,
             false);
}

void RoadRunner::addData2(
    const QVector<QGeoCoordinate>& visibleCoords,
    bool,
    const QGeoCoordinate& planeCoord,
    const QGeoCoordinate&,
    const QGeoCoordinate& viewCenter,
    const int prevWPPlanIdx,
    const int nextWPPlanIdx,
    const QGeoCoordinate&,
    const QGeoCoordinate&,
    int,
    const float planeYaw,
    const float viewAngle,
    const float viewDist,
    const float currentYawOffset,
    const float maxOffset,
    const bool isExactMagValue,
    bool)
{
    if (!enabled_ || map_ == nullptr || !planeCoord.isValid()) {
        return;
    }
    RoadValidateOffset offset;
    QGeoCoordinate projected;
    int itemIndex = -1;
    const float distance =
        roadProjCoord(planeCoord, projected, offset, itemIndex);
    RoadValidateResult result;
    result.time = QDateTime::currentMSecsSinceEpoch();
    result.objectIdx = itemIndex;
    result.roadItem = itemIndex;
    result.offset = distance;
    result.offsetX = static_cast<float>(offset.east);
    result.offsetY = static_cast<float>(offset.north);
    result.coordinate = projected;
    const float denominator = std::max(maxOffset, 1.0F);
    result.confidence = std::clamp(1.0F - distance / denominator,
                                   0.0F,
                                   1.0F);
    result.accepted = result.confidence >= minimumConfidence_
        && std::abs(currentYawOffset) <= maximumYawOffsetDegrees_;
    result.temporary = !isExactMagValue;
    result.roadItems.append(itemIndex);
    pendingOffsets_.append(result);
    roadItemDetectionTimes_[prevWPPlanIdx] = result.time;
    roadItemDetectionTimes_[nextWPPlanIdx] = result.time;
    emit offsetItemAdded(result);

    if (searchRoadElementsEnabled_) {
        QVector<QPointF> locals;
        locals.reserve(visibleCoords.size());
        for (const QGeoCoordinate& coordinate : visibleCoords) {
            QPointF local;
            map_->toLocalCoord(coordinate, local);
            locals.append(local);
        }
        QPointF planeLocal;
        map_->toLocalCoord(planeCoord, planeLocal);
        analyzer_->searchElements(planeCoord,
                                  planeLocal,
                                  viewCenter,
                                  locals,
                                  prevWPPlanIdx,
                                  nextWPPlanIdx,
                                  planeYaw,
                                  viewAngle,
                                  isExactMagValue);
    }
    if (viewDist >= 0.0F) {
        calculateAverageOffset();
    }
}

void RoadRunner::timerEvent(QTimerEvent* const event)
{
    if (event->timerId() == matchTimerId_) {
        match();
        return;
    }
    QObject::timerEvent(event);
}

void RoadRunner::detectedElementsHandler(
    const QVector<RoadElement>& elements)
{
    detectedElements_ = elements;
    const qlonglong now = QDateTime::currentMSecsSinceEpoch();
    QList<RoadElement*> pointers;
    if (map_ != nullptr) {
        for (const RoadElement& detected : elements) {
            if (detected.index >= 0 && detected.index < map_->elements.size()) {
                RoadElement& element = map_->elements[detected.index];
                element.lastDetectedTime = now;
                pointers.append(&element);
            }
        }
    }
    emit elementsDetected(now, pointers);
}

bool RoadRunner::headingInRange(const float heading,
                                const float minimum,
                                const float maximum)
{
    if (minimum <= maximum) {
        return heading >= minimum && heading <= maximum;
    }
    return heading >= minimum || heading <= maximum;
}

void RoadRunner::calculateAverageOffset()
{
    if (pendingOffsets_.isEmpty()) {
        return;
    }
    double sumX = 0.0;
    double sumY = 0.0;
    double sumConfidence = 0.0;
    for (const RoadValidateResult& item : pendingOffsets_) {
        sumX += item.offsetX;
        sumY += item.offsetY;
        sumConfidence += item.confidence;
    }
    RoadValidateResult result = pendingOffsets_.last();
    const double count = static_cast<double>(pendingOffsets_.size());
    result.offsetX = static_cast<float>(sumX / count);
    result.offsetY = static_cast<float>(sumY / count);
    result.offset = static_cast<float>(std::hypot(result.offsetX,
                                                   result.offsetY));
    result.confidence = static_cast<float>(sumConfidence / count);
    result.accepted = result.confidence >= minimumConfidence_;
    currentOffset_ = result;
    emit offsetChanged(result);
}

