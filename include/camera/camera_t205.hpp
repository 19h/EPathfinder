#pragma once

#include "camera/camera_v4l.hpp"

class CameraT205 final : public CameraV4L {
    Q_OBJECT

public:
    explicit CameraT205(const QString& device,
                        QObject* parent = nullptr);
    ~CameraT205() override = default;
};

