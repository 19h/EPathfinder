#pragma once

#include "camera/camera_usb.hpp"

#include <QProcess>

class CameraV4L : public CameraUSB {
    Q_OBJECT

public:
    explicit CameraV4L(const QString& device,
                       QObject* parent = nullptr);
    ~CameraV4L() override = default;

    void setZoom(float percent) override;
    void setFocus(float distance,
                  float zoomPercent,
                  int timeoutMSec) override;

protected:
    void timerEvent(QTimerEvent* event) override;

private slots:
    void initStartZoom100();
    void initStartZoom0();
    void readCurrentZoom();

private:
    int zoomMaximum_{500};
    int zoomMinimum_{200};
    bool initializingZoom_{};
    qlonglong lastZoomCommandTime_{};
    QProcess* zoomProcess_{};
};

