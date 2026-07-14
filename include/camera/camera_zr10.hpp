#pragma once

#include "camera/camera.hpp"

#include <QByteArray>
#include <QProcess>
#include <QString>

class QTimerEvent;
class QUdpSocket;
class SerialLink;

class CameraZR10 final : public Camera {
    Q_OBJECT

public:
    CameraZR10(const QString& address,
               int localPort,
               int remotePort,
               bool externalGimbalControl,
               QObject* parent = nullptr);
    ~CameraZR10() override = default;

    void connect() override;
    void disconnect() override;
    void setZoom(float percent) override;
    void setFocus(float distance,
                  float zoomPercent,
                  int timeoutMSec) override;

    void setGimbal(const Gimbal& value);
    void setAutoCentering(bool enabled);
    void setSerialLink(const QString& portName, int baudRate);
    unsigned short CRC16_cal(unsigned char* data,
                             unsigned int length,
                             unsigned short initialValue);

protected:
    void timerEvent(QTimerEvent* event) override;

private slots:
    void ping();
    void pingFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void readPendingDatagrams();
    void setAutoFocus();
    void serialLinkDataReceived(const QByteArray& data);

private:
    static constexpr unsigned char kStartLow = 0x55;
    static constexpr unsigned char kStartHigh = 0x66;
    static constexpr unsigned char kVersion = 0x01;
    static constexpr unsigned char kZoomCommand = 0x0f;
    static constexpr unsigned char kAutoFocusCommand = 0x04;
    static constexpr unsigned char kAutoCenterCommand = 0x08;
    static constexpr unsigned char kReadGimbalCommand = 0x0d;
    static constexpr unsigned char kSetGimbalCommand = 0x0e;

    QByteArray makePacket(unsigned char command, const QByteArray& payload);
    void sendPacket(const QByteArray& packet);
    void readGimbal();
    void parseData(qlonglong time, int size);
    void parseFrame(const QByteArray& frame, qlonglong time);
    void appendCalibrations();

    QUdpSocket* udpSocket_{};
    QString address_;
    int localPort_{};
    int remotePort_{};
    SerialLink* serialLink_{};
    bool serialTransport_{};
    QByteArray serialBuffer_;
    QProcess* pingProcess_{};
    bool autoCentering_{};
    unsigned short sequence_{};
    qlonglong gimbalReadIntervalMs_{30};
    qlonglong lastGimbalMessageMs_{};
    QByteArray receiveData_;
    bool externalGimbalControl_{};
    int zoomTimestampCorrectionMs_{10};
    bool zoomCommandInitialized_{};
};
