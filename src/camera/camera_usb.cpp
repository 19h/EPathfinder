#include "camera/camera_usb.hpp"

#include <QFileInfo>
#include <QTimer>

CameraUSB::CameraUSB(const QString& device, QObject* parent)
    : Camera(parent)
    , device_(device)
{
    QTimer::singleShot(checkIntervalMs_, this, &CameraUSB::checkDevice);
}

void CameraUSB::connect()
{
    if (deviceIsEnabled()) {
        setConnectedState(true);
    }
}

void CameraUSB::disconnect()
{
    setConnectedState(false);
}

void CameraUSB::checkDevice()
{
    const bool available = QFileInfo::exists(device_);
    setDeviceEnabledState(available);
    if (!available) {
        disconnect();
    }
    QTimer::singleShot(checkIntervalMs_, this, &CameraUSB::checkDevice);
}

