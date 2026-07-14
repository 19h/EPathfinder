#pragma once

#include <QVector3D>

// Original symbol at RVA 0x42c20. All positions share one Cartesian frame.
// Returns a null vector when the ray is parallel to the plane, matching the
// binary's exact denominator == 0 behavior.
QVector3D rayPlaneIntersection(const QVector3D& planePoint,
                               const QVector3D& planeNormal,
                               const QVector3D& rayOrigin,
                               const QVector3D& rayDirection) noexcept;

