#pragma once

#include <QMatrix4x4>
#include <QPoint>
#include <QPointF>
#include <QString>
#include <QVector>

#include <cstddef>

inline constexpr int kZoomMapWidth = 1920;
inline constexpr int kZoomMapHeight = 1080;
inline constexpr std::size_t kZoomMatrixElements = 9;
inline constexpr std::size_t kZoomPlaneElements =
    static_cast<std::size_t>(kZoomMapWidth) * kZoomMapHeight;
inline constexpr std::size_t kZoomMapFloatCount =
    kZoomMatrixElements + 2 * kZoomPlaneElements;
inline constexpr std::size_t kZoomMapBytes =
    kZoomMapFloatCount * sizeof(float);
static_assert(kZoomMapBytes == 0xFD2024U);

extern float zoom_00[kZoomMapFloatCount];
extern float zoom_05[kZoomMapFloatCount];
extern float zoom_10[kZoomMapFloatCount];
extern float zoom_15[kZoomMapFloatCount];
extern float zoom_20[kZoomMapFloatCount];
extern QString data_path;
extern bool init;

enum class ZoomMap : int {
    Zoom00 = 0,
    Zoom05 = 5,
    Zoom10 = 10,
    Zoom15 = 15,
    Zoom20 = 20,
};

double lin(double x,
           double lowerX,
           double lowerY,
           double upperX,
           double upperY) noexcept;

// Clamps zoom to [0, 20] and returns its adjacent 5-unit calibration bracket.
void calculate_zoom(double& zoom, double& lowerZoom, double& upperZoom) noexcept;

float* get_zoom_data(double zoom) noexcept;
bool read_data(QString path, void* destination, int expectedSize);
bool init_remap();

QMatrix4x4 createQMatrix4x4(double m11,
                            double m12,
                            double m13,
                            double m21,
                            double m22,
                            double m23,
                            double m31,
                            double m32,
                            double m33) noexcept;

QMatrix4x4 get_transform(float (&calibration)[3][3],
                         double scale,
                         double angleZDegrees,
                         double angleXDegrees,
                         double angleYDegrees);

QVector<QPointF> reproject_impl(double zoom,
                                double scale,
                                double angleZDegrees,
                                double angleXDegrees,
                                double angleYDegrees,
                                QVector<QPoint> points);

QVector<QPointF> reproject(double zoom,
                           double scale,
                           double angleZDegrees,
                           double angleXDegrees,
                           double angleYDegrees,
                           QVector<QPoint> points);

// Recovered abstraction of get_zoom_data(double). The original returns one of
// five runtime-filled BSS arrays (16,588,836 bytes each); this function exposes
// the selection without inventing unavailable calibration files.
ZoomMap selectZoomMap(double zoom) noexcept;
