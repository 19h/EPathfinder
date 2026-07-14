#include "camera/gimbal_transform.hpp"

#include <QVector3D>

QQuaternion gimbalMTX(const RPY gimbal) noexcept
{
    const QQuaternion yaw = QQuaternion::fromAxisAndAngle(
        QVector3D{0.0F, 1.0F, 0.0F}, gimbal.yaw);
    const QQuaternion roll = QQuaternion::fromAxisAndAngle(
        QVector3D{0.0F, 0.0F, 1.0F}, gimbal.roll);
    const QQuaternion pitch = QQuaternion::fromAxisAndAngle(
        QVector3D{1.0F, 0.0F, 0.0F}, gimbal.pitch);
    // The optimized AArch64 body at RVA 0x49840 composes Y * Z * X.
    return yaw * roll * pitch;
}

QQuaternion e2q(const RPY euler) noexcept
{
    // Qt uses pitch, yaw, roll argument order and degrees.
    return QQuaternion::fromEulerAngles(euler.pitch, euler.yaw, euler.roll);
}

RPY q2e(const QQuaternion quaternion) noexcept
{
    RPY result;
    quaternion.getEulerAngles(&result.pitch, &result.yaw, &result.roll);
    return result;
}

RPY body2camera(const RPY body, const RPY gimbal) noexcept
{
    return q2e(e2q(body) * gimbalMTX(gimbal));
}

RPY camera2body(const RPY camera, const RPY gimbal) noexcept
{
    return q2e(e2q(camera) * gimbalMTX(gimbal).inverted());
}
