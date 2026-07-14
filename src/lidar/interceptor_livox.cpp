#include "lidar/interceptor_livox.hpp"

#include "lidar/lds_lidar.hpp"

#include <QVector3D>

#include <cstddef>

static_assert(sizeof(PlaneCoord) == 12U);
static_assert(sizeof(LivoxPoint) == 13U);

InterceptorLivox::InterceptorLivox(QObject* const parent)
    : InterceptorLidar(parent)
    , frame_time_(100)
    , cluster_distance_squared_(100.0F)
{
    qRegisterMetaType<QVector3D>();
}

void InterceptorLivox::Init()
{
    auto& source = LdsLidar::GetInstance();
    source.InitLdsLidar(broadcast_codes_);
    source.SetQDataCallback(
        [this](const QList<LivoxPoint>& points) { livoxDataCallback(points); });
}

void InterceptorLivox::setFrameTime(const int frameTime)
{
    frame_time_ = frameTime;
}

int InterceptorLivox::frameTime() const
{
    return frame_time_;
}

void InterceptorLivox::livoxDataCallback(const QList<LivoxPoint>& points)
{
    QList<QList<LivoxPoint>> clusters;
    for (const auto& point : points) {
        bool added = false;
        for (auto& cluster : clusters) {
            const auto& anchor = cluster.first();
            const QVector3D difference(anchor.x - point.x,
                                       anchor.y - point.y,
                                       anchor.z - point.z);
            if (cluster_distance_squared_ > difference.lengthSquared()) {
                cluster.append(point);
                added = true;
                break;
            }
        }
        if (!added) {
            clusters.append(QList<LivoxPoint>{point});
        }
    }

    QList<PlaneCoord> objects;
    for (const auto& cluster : clusters) {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
        for (const auto& point : cluster) {
            x += point.x;
            y += point.y;
            z += point.z;
        }
        const float count = static_cast<float>(cluster.size());
        objects.append(PlaneCoord{x / count, y / count, z / count});
    }
    emit lidarData(objects);
}

