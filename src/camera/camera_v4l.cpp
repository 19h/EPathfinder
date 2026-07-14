#include "camera/camera_v4l.hpp"

#include <QDateTime>
#include <QFileInfo>
#include <QTimer>
#include <QTimerEvent>

#include <algorithm>

CameraV4L::CameraV4L(const QString& device, QObject* parent)
    : CameraUSB(device, parent)
{
    QTimer::singleShot(0, this, &CameraV4L::initStartZoom100);
    startTimer(200);
}

void CameraV4L::initStartZoom100()
{
    initializingZoom_ = true;
    setZoom(100.0F);
    QTimer::singleShot(200, this, &CameraV4L::initStartZoom0);
}

void CameraV4L::initStartZoom0()
{
    setZoom(0.0F);
    initializingZoom_ = false;
}

void CameraV4L::setFocus(float distance,
                         float zoomPercent,
                         int timeoutMSec)
{
    Q_UNUSED(distance)
    Q_UNUSED(zoomPercent)
    Q_UNUSED(timeoutMSec)
}

void CameraV4L::setZoom(float percent)
{
    percent = std::clamp(percent, 0.0F, 100.0F);
    updateZoom(percent);
    lastZoomCommandTime_ = QDateTime::currentMSecsSinceEpoch();
    if (!device_.isEmpty() && QFileInfo::exists(device_)) {
        if (!zoomProcess_) {
            zoomProcess_ = new QProcess(this);
        }
        const int value = qRound(zoomMinimum_ +
                                 (zoomMaximum_ - zoomMinimum_) *
                                     percent / 100.0F);
        zoomProcess_->start(QStringLiteral("v4l2-ctl"),
                            {QStringLiteral("-d"), device_,
                             QStringLiteral("--set-ctrl=zoom_absolute=%1")
                                 .arg(value)});
    }
}

void CameraV4L::readCurrentZoom()
{
    if (!zoomProcess_ || zoomProcess_->state() != QProcess::NotRunning) {
        return;
    }
}

void CameraV4L::timerEvent(QTimerEvent* event)
{
    Q_UNUSED(event)
    readCurrentZoom();
}
