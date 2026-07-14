#include "road/road_types.hpp"

#include <QGeoPath>
#include <QLineF>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr double kDegreesToRadians = 0.017453292519943295769;

float normalizedHeading(float heading)
{
    if (heading < 0.0F) {
        heading += 360.0F;
    } else if (heading >= 360.0F) {
        heading -= 360.0F;
    }
    return heading;
}

} // namespace

void RoadElement::addItem(const float headingDegrees,
                          const QVector<QPointF>& points)
{
    headings.append(normalizedHeading(headingDegrees));
    if (points.isEmpty()) {
        const double radians = headingDegrees * kDegreesToRadians;
        localItems.append({QPointF(std::sin(radians) * 1000.0,
                                   std::cos(radians) * 1000.0)});
        itemLengths.append(1000.0F);
        return;
    }

    localItems.append(points);
    float length = 0.0F;
    QPointF previous;
    for (const QPointF& point : points) {
        length += static_cast<float>(QLineF(previous, point).length());
        previous = point;
    }
    itemLengths.append(length);
}

QGeoCoordinate RoadMap::calcCenter()
{
    QGeoPath path;
    for (const RoadPoint& point : points) {
        path.addCoordinate(point.coordinate);
    }
    center = path.center();
    return center;
}

void RoadMap::toLocalCoord(const QGeoCoordinate& coordinate,
                           QPointF& local) const
{
    const double distanceMetres = center.distanceTo(coordinate);
    const double azimuthRadians =
        center.azimuthTo(coordinate) * kDegreesToRadians;
    local.setX(std::sin(azimuthRadians) * distanceMetres);
    local.setY(std::cos(azimuthRadians) * distanceMetres);
}

float RoadMap::mapNearestDistance(const QPointF& local,
                                  QPointF& projected,
                                  int& itemIndex) const
{
    float minimum = std::numeric_limits<float>::max();
    for (const RoadItem& item : items) {
        if (item.point0 < 0 || item.point0 >= points.size()
            || item.point1 < 0 || item.point1 >= points.size()) {
            continue;
        }

        const QPointF first = points.at(item.point0).localCoordinate;
        const QPointF delta = points.at(item.point1).localCoordinate - first;
        const double squaredLength = QPointF::dotProduct(delta, delta);
        QPointF candidate = first;
        if (squaredLength >= 1.1921e-7) {
            const double factor = std::clamp(
                QPointF::dotProduct(local - first, delta) / squaredLength,
                0.0,
                1.0);
            candidate += delta * factor;
        }
        const float distance =
            static_cast<float>(QLineF(local, candidate).length());
        if (distance < minimum) {
            minimum = distance;
            projected = candidate;
            itemIndex = item.index;
        }
    }
    return minimum;
}

float RoadMap::projCoord(const QGeoCoordinate& coordinate,
                         QGeoCoordinate& projected,
                         RoadValidateOffset& offset,
                         int& itemIndex) const
{
    itemIndex = -1;
    QPointF local;
    QPointF projectedLocal;
    toLocalCoord(coordinate, local);
    const float distance =
        mapNearestDistance(local, projectedLocal, itemIndex);
    offset.east = projectedLocal.x() - local.x();
    offset.north = projectedLocal.y() - local.y();
    projected = coordinate.atDistanceAndAzimuth(offset.east, 90.0);
    projected = projected.atDistanceAndAzimuth(offset.north, 0.0);
    return distance;
}

bool RoadMap::checkNewCenter(const QGeoCoordinate& coordinate,
                             const float thresholdMetres)
{
    if (!center.isValid() || points.isEmpty()
        || coordinate.distanceTo(center) < thresholdMetres) {
        return false;
    }
    center = coordinate;
    for (RoadPoint& point : points) {
        toLocalCoord(point.coordinate, point.localCoordinate);
    }
    for (RoadItem& item : items) {
        if (item.point0 < 0 || item.point0 >= points.size()
            || item.point1 < 0 || item.point1 >= points.size()) {
            continue;
        }
        const QPointF delta = points.at(item.point1).localCoordinate
            - points.at(item.point0).localCoordinate;
        item.heading = normalizedHeading(static_cast<float>(
            std::atan2(delta.x(), delta.y()) / kDegreesToRadians));
    }
    return true;
}

void RoadMap::clear()
{
    points.clear();
    items.clear();
    elements.clear();
    center = QGeoCoordinate();
    outgoing.clear();
    incoming.clear();
}

void RoadMap::fromCoords(const QVector<QGeoCoordinate>& coordinates)
{
    clear();
    points.reserve(coordinates.size());
    for (int index = 0; index < coordinates.size(); ++index) {
        RoadPoint point;
        point.number = index + 1;
        point.coordinate = coordinates.at(index);
        points.append(point);
    }
    calcCenter();
    for (RoadPoint& point : points) {
        toLocalCoord(point.coordinate, point.localCoordinate);
    }

    items.reserve(std::max<qsizetype>(0, points.size() - 1));
    for (int index = 0; index + 1 < points.size(); ++index) {
        RoadItem item;
        item.index = index;
        item.point0 = index;
        item.point1 = index + 1;
        item.planItemIndex = points.at(index).number;
        item.length = static_cast<float>(
            points.at(index).coordinate.distanceTo(
                points.at(index + 1).coordinate));
        items.append(item);
        outgoing[index].append(index + 1);
        incoming[index + 1].append(index);
    }
    // The binary computes headings after establishing the center.
    for (RoadItem& item : items) {
        const QPointF delta = points.at(item.point1).localCoordinate
            - points.at(item.point0).localCoordinate;
        item.heading = normalizedHeading(static_cast<float>(
            std::atan2(delta.x(), delta.y()) / kDegreesToRadians));
    }
}
