#include "vision/thunder_gaze.hpp"

#include <QSet>

#include <algorithm>
#include <cmath>

ThunderGaze::ThunderGaze(QObject* const parent, QUdpSocket* const socket)
    : QObject(parent)
    , socket_(socket)
{
}

void ThunderGaze::setFrameDelay(const int milliseconds)
{
    frameDelayMs_ = milliseconds;
}

void ThunderGaze::setPTZOffset(const float horizontalDegrees,
                               const float verticalDegrees)
{
    ptzOffsetX_ = horizontalDegrees;
    ptzOffsetY_ = verticalDegrees;
}

void ThunderGaze::setDebugDTPEnabled(const bool enabled)
{
    debugDtpEnabled_ = enabled;
}

void ThunderGaze::setDetTypeEnabled(const int index, const bool enabled)
{
    if (index >= 0 && index < 4) {
        detectionTypes_[index] = enabled;
    }
}

void ThunderGaze::setRoadsEnabled(const bool enabled)
{
    roadsEnabled_ = enabled;
}

void ThunderGaze::setRoadSimpleMode(const bool enabled)
{
    roadSimpleMode_ = enabled;
}

void ThunderGaze::setCamera(Camera* const camera)
{
    camera_ = camera;
}

void ThunderGaze::setReduceBorderEnabled(const bool enabled)
{
    reduceBorderEnabled_ = enabled;
}

void ThunderGaze::setIsRailway(const unsigned char value)
{
    railway_ = value;
}

void ThunderGaze::setPreprocessingScale(float)
{
    // Protocol V1 has no command channel for this operation.
}

void ThunderGaze::setFrameText(const QStringList&)
{
    // Protocols V1 and V2 do not expose frame text.
}

void ThunderGaze::setRTSPEnabled(bool)
{
    // Protocols V1--V3 do not expose RTSP control.
}

void ThunderGaze::setTelemetryParams(const QJsonDocument&)
{
    // Protocols V1--V3 do not expose telemetry configuration.
}

void ThunderGaze::setObjectDetectorEnabled(bool)
{
    // Protocols V1--V3 do not expose this control.
}

void ThunderGaze::setPathfinderEnabled(bool)
{
    // Protocols V1--V3 do not expose this control.
}

void ThunderGaze::setVideoSaverEnabled(bool)
{
    // Protocols V1--V4 do not expose this control.
}

void ThunderGaze::splitRoadScreenParts(const qlonglong time,
                                       const QVector<QPoint>& points)
{
    if (camera_ == nullptr || points.isEmpty()) {
        return;
    }
    const int width = camera_->currentParams().frameWidth;
    const int oneThird = width / 3;
    const int twoThirds = 2 * width / 3;
    int left = 0;
    int right = 0;
    for (const QPoint& point : points) {
        if (point.x() < oneThird) {
            ++left;
        } else if (point.x() >= twoThirds) {
            ++right;
        }
    }
    // The binary passes the total point count as the parameter named middle.
    emit roadScreenParts(time, left, points.size(), right);
}

void ThunderGaze::clearIgnores()
{
    ignoreBounds_.clear();
}

void ThunderGaze::addIgnoreBound(const IgnoreBound& bound)
{
    ignoreBounds_.append(bound);
}

void ThunderGaze::reduceRoadPoints(QVector<QPoint>& points) const
{
    if (camera_ == nullptr || points.isEmpty()) {
        return;
    }
    const CameraParams params = camera_->currentParams();
    if (params.frameWidth <= 0 || params.frameHeight <= 0) {
        points.clear();
        return;
    }

    QSet<quint64> occupied;
    QVector<QPoint> reduced;
    reduced.reserve(points.size());
    for (const QPoint& point : points) {
        if (point.x() < 0 || point.y() < 0
            || point.x() >= params.frameWidth
            || point.y() >= params.frameHeight) {
            continue;
        }
        bool ignored = false;
        for (const IgnoreBound& bound : ignoreBounds_) {
            if (point.x() >= bound.lowerX && point.x() <= bound.upperX
                && point.y() >= bound.lowerY && point.y() <= bound.upperY) {
                ignored = true;
                break;
            }
        }
        if (ignored) {
            continue;
        }
        if (reduceBorderEnabled_
            && (point.x() == 0 || point.y() == 0
                || point.x() == params.frameWidth - 1
                || point.y() == params.frameHeight - 1)) {
            continue;
        }
        const int column = std::min(reductionColumns_ - 1,
                                    point.x() * reductionColumns_
                                        / params.frameWidth);
        const int row = std::min(reductionRows_ - 1,
                                 point.y() * reductionRows_
                                     / params.frameHeight);
        const quint64 key = (static_cast<quint64>(row) << 32U)
            | static_cast<quint32>(column);
        if (!occupied.contains(key)) {
            occupied.insert(key);
            reduced.append(point);
        }
    }
    points.swap(reduced);
}

void ThunderGaze::reduceRoadPointsSimpleMode(QVector<QPoint>& points) const
{
    reduceRoadPoints(points);
    std::sort(points.begin(), points.end(), [](const QPoint& first,
                                               const QPoint& second) {
        return first.y() == second.y() ? first.x() < second.x()
                                       : first.y() < second.y();
    });
}

void ThunderGaze::calculateRoadPointAngles(
    const CameraParams& params,
    const QVector<QPoint>& points,
    QVector<RoadPointAngle>& output) const
{
    output.clear();
    if (camera_ == nullptr) {
        return;
    }
    const CameraParams current = camera_->currentParams();
    if (current.frameWidth == 0 || current.frameHeight == 0) {
        return;
    }
    output.reserve(points.size());
    for (const QPoint& point : points) {
        const float normalizedX = static_cast<float>(point.x())
                / static_cast<float>(current.frameWidth)
            - 0.5F;
        const float normalizedY = static_cast<float>(point.y())
                / static_cast<float>(current.frameHeight)
            - 0.5F;
        RoadPointAngle result;
        result.pixelX = static_cast<std::int16_t>(point.x());
        result.pixelY = static_cast<std::int16_t>(point.y());
        result.angleX =
            ptzOffsetX_ + params.zoom.widthAngle * normalizedX;
        result.angleY =
            ptzOffsetY_ - params.zoom.heightAngle * normalizedY;
        output.append(result);
    }
}

bool ThunderGaze::acceptsMessage(const QByteArray& message) const
{
    return message.size() == messageSize();
}

