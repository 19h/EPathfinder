#pragma once

#include <QQuaternion>

struct RPY {
    float roll = 0.0F;
    float pitch = 0.0F;
    float yaw = 0.0F;
};

QQuaternion gimbalMTX(RPY gimbal) noexcept;
QQuaternion e2q(RPY euler) noexcept;
RPY q2e(QQuaternion quaternion) noexcept;
RPY body2camera(RPY body, RPY gimbal) noexcept;
RPY camera2body(RPY camera, RPY gimbal) noexcept;

