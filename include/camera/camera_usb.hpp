#pragma once

#include "camera/camera.hpp"

#include <QString>

class CameraUSB : public Camera {
    Q_OBJECT

public:
    explicit CameraUSB(const QString& device,
                       QObject* parent = nullptr);
    ~CameraUSB() override = default;

    void connect() override;
    void disconnect() override;

private slots:
    void checkDevice();

protected:
    QString device_;
    int checkIntervalMs_{10};
};

