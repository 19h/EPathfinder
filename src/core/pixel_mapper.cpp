#include "core/pixel_mapper.hpp"

#include <QDebug>
#include <QFile>

#include <cmath>
#include <cstring>

float zoom_00[kZoomMapFloatCount]{};
float zoom_05[kZoomMapFloatCount]{};
float zoom_10[kZoomMapFloatCount]{};
float zoom_15[kZoomMapFloatCount]{};
float zoom_20[kZoomMapFloatCount]{};
QString data_path = QStringLiteral("H:/maps/zoom");
bool init = false;

double lin(const double x,
           const double lowerX,
           const double lowerY,
           const double upperX,
           const double upperY) noexcept
{
    return ((x - lowerX) * upperY + (upperX - x) * lowerY)
        / (upperX - lowerX);
}

void calculate_zoom(double& zoom,
                    double& lowerZoom,
                    double& upperZoom) noexcept
{
    if (zoom < 0.0) {
        zoom = 0.0;
        lowerZoom = 0.0;
        upperZoom = 5.0;
    } else if (zoom > 20.0) {
        zoom = 20.0;
        lowerZoom = 15.0;
        upperZoom = 20.0;
    } else if (zoom < 5.0) {
        lowerZoom = 0.0;
        upperZoom = 5.0;
    } else if (zoom < 10.0) {
        lowerZoom = 5.0;
        upperZoom = 10.0;
    } else if (zoom < 15.0) {
        lowerZoom = 10.0;
        upperZoom = 15.0;
    } else {
        lowerZoom = 15.0;
        upperZoom = 20.0;
    }
}

ZoomMap selectZoomMap(const double zoom) noexcept
{
    if (zoom > 17.0) {
        return ZoomMap::Zoom20;
    }
    if (zoom > 12.0) {
        return ZoomMap::Zoom15;
    }
    if (zoom > 7.0) {
        return ZoomMap::Zoom10;
    }
    if (zoom > 2.0) {
        return ZoomMap::Zoom05;
    }
    return ZoomMap::Zoom00;
}

float* get_zoom_data(const double zoom) noexcept
{
    switch (selectZoomMap(zoom)) {
    case ZoomMap::Zoom20:
        return zoom_20;
    case ZoomMap::Zoom15:
        return zoom_15;
    case ZoomMap::Zoom10:
        return zoom_10;
    case ZoomMap::Zoom05:
        return zoom_05;
    case ZoomMap::Zoom00:
        return zoom_00;
    }
    return zoom_00;
}

QMatrix4x4 createQMatrix4x4(const double m11,
                            const double m12,
                            const double m13,
                            const double m21,
                            const double m22,
                            const double m23,
                            const double m31,
                            const double m32,
                            const double m33) noexcept
{
    return QMatrix4x4(static_cast<float>(m11),
                      static_cast<float>(m12),
                      static_cast<float>(m13),
                      0.0F,
                      static_cast<float>(m21),
                      static_cast<float>(m22),
                      static_cast<float>(m23),
                      0.0F,
                      static_cast<float>(m31),
                      static_cast<float>(m32),
                      static_cast<float>(m33),
                      0.0F,
                      0.0F,
                      0.0F,
                      0.0F,
                      1.0F);
}

QMatrix4x4 get_transform(float (&calibration)[3][3],
                         const double scale,
                         const double angleZDegrees,
                         const double angleXDegrees,
                         const double angleYDegrees)
{
    // The decimal pi literal is preserved from RVA 0x4a0c8.
    constexpr double degreesToRadians = 3.14159265 / 180.0;
    const double sinY = std::sin(angleYDegrees * degreesToRadians);
    const double cosY = std::cos(angleYDegrees * degreesToRadians);
    const double sinX = std::sin(angleXDegrees * degreesToRadians);
    const double cosX = std::cos(angleXDegrees * degreesToRadians);
    const double sinZ = std::sin(angleZDegrees * degreesToRadians);
    const double cosZ = std::cos(angleZDegrees * degreesToRadians);

    const QMatrix4x4 camera = createQMatrix4x4(calibration[0][0],
                                               calibration[0][1],
                                               calibration[0][2],
                                               calibration[1][0],
                                               calibration[1][1],
                                               calibration[1][2],
                                               calibration[2][0],
                                               calibration[2][1],
                                               calibration[2][2]);
    const QMatrix4x4 zoom = createQMatrix4x4(-scale,
                                             0.0,
                                             0.0,
                                             0.0,
                                             -scale,
                                             0.0,
                                             0.0,
                                             0.0,
                                             1.0);
    const QMatrix4x4 axisConversion = createQMatrix4x4(1.0,
                                                       0.0,
                                                       0.0,
                                                       0.0,
                                                       0.0,
                                                       -1.0,
                                                       0.0,
                                                       -1.0,
                                                       0.0);
    const QMatrix4x4 rotationY = createQMatrix4x4(cosY,
                                                  0.0,
                                                  sinY,
                                                  0.0,
                                                  1.0,
                                                  0.0,
                                                  -sinY,
                                                  0.0,
                                                  cosY);
    const QMatrix4x4 rotationX = createQMatrix4x4(1.0,
                                                  0.0,
                                                  0.0,
                                                  0.0,
                                                  cosX,
                                                  -sinX,
                                                  0.0,
                                                  sinX,
                                                  cosX);
    const QMatrix4x4 rotationZ = createQMatrix4x4(cosZ,
                                                  -sinZ,
                                                  0.0,
                                                  sinZ,
                                                  cosZ,
                                                  0.0,
                                                  0.0,
                                                  0.0,
                                                  1.0);

    return zoom * axisConversion.inverted() * rotationY * rotationX
        * rotationZ * camera.inverted();
}

QVector<QPointF> reproject_impl(const double zoom,
                                const double scale,
                                const double angleZDegrees,
                                const double angleXDegrees,
                                const double angleYDegrees,
                                const QVector<QPoint> points)
{
    QVector<QPointF> result;
    float* const zoomData = get_zoom_data(zoom);
    auto& calibration = *reinterpret_cast<float (*)[3][3]>(zoomData);
    const QMatrix4x4 transform = get_transform(calibration,
                                               scale,
                                               angleZDegrees,
                                               angleXDegrees,
                                               angleYDegrees);
    const float* const matrix = transform.constData();

    for (const QPoint& point : points) {
        const std::size_t pixel = static_cast<std::size_t>(kZoomMapWidth)
                * static_cast<std::size_t>(point.y())
            + static_cast<std::size_t>(point.x());
        const float mapX = zoomData[kZoomMatrixElements + pixel];
        const float mapY =
            zoomData[kZoomMatrixElements + kZoomPlaneElements + pixel];
        const float denominator = mapY * matrix[6] + mapX * matrix[2]
            + matrix[10] + matrix[14] * 0.0F;
        const float projectedX = (mapY * matrix[4] + mapX * matrix[0]
                                  + matrix[8] + matrix[12] * 0.0F)
            / denominator;
        const float projectedY = (mapY * matrix[5] + mapX * matrix[1]
                                  + matrix[9] + matrix[13] * 0.0F)
            / denominator;
        result.append(QPointF(projectedX, projectedY));
    }
    return result;
}

QVector<QPointF> reproject(double zoom,
                           const double scale,
                           const double angleZDegrees,
                           const double angleXDegrees,
                           const double angleYDegrees,
                           const QVector<QPoint> points)
{
    QVector<QPointF> result;
    if (!init) {
        return result;
    }

    double lowerZoom = 0.0;
    double upperZoom = 0.0;
    calculate_zoom(zoom, lowerZoom, upperZoom);
    const QVector<QPointF> lower = reproject_impl(lowerZoom,
                                                  scale,
                                                  angleZDegrees,
                                                  angleXDegrees,
                                                  angleYDegrees,
                                                  points);
    const QVector<QPointF> upper = reproject_impl(upperZoom,
                                                  scale,
                                                  angleZDegrees,
                                                  angleXDegrees,
                                                  angleYDegrees,
                                                  points);
    for (int index = 0; index < points.size(); ++index) {
        result.append(QPointF(lin(zoom,
                                  lowerZoom,
                                  lower[index].x(),
                                  upperZoom,
                                  upper[index].x()),
                              lin(zoom,
                                  lowerZoom,
                                  lower[index].y(),
                                  upperZoom,
                                  upper[index].y())));
    }
    return result;
}

bool read_data(const QString path, void* destination, const int expectedSize)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Read resource error!";
        return false;
    }

    const QByteArray bytes = file.readAll();
    if (bytes.size() != expectedSize) {
        qDebug() << "Resource size error!";
        return false;
    }
    std::memcpy(destination, bytes.constData(), static_cast<std::size_t>(expectedSize));
    return true;
}

bool init_remap()
{
    if (!QFile::exists(data_path + QStringLiteral("/ok"))) {
        return false;
    }

    init = true;
    const auto size = static_cast<int>(kZoomMapBytes);
    init = init && read_data(data_path + QStringLiteral("/zoom_00.dat"), zoom_00, size);
    init = init && read_data(data_path + QStringLiteral("/zoom_05.dat"), zoom_05, size);
    init = init && read_data(data_path + QStringLiteral("/zoom_10.dat"), zoom_10, size);
    init = init && read_data(data_path + QStringLiteral("/zoom_15.dat"), zoom_15, size);
    init = init && read_data(data_path + QStringLiteral("/zoom_20.dat"), zoom_20, size);
    return init;
}
