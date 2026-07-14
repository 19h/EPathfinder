#include "core/emath.hpp"

QVector3D rayPlaneIntersection(const QVector3D& planePoint,
                               const QVector3D& planeNormal,
                               const QVector3D& rayOrigin,
                               const QVector3D& rayDirection) noexcept
{
    const float numerator = QVector3D::dotProduct(
        planePoint - rayOrigin, planeNormal);
    const float denominator = QVector3D::dotProduct(
        rayDirection, planeNormal);
    if (denominator == 0.0F) {
        return {};
    }
    return rayOrigin + rayDirection * (numerator / denominator);
}

