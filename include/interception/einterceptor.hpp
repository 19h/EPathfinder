#pragma once

#include "elink/elink_telemetry.hpp"
#include "lidar/interceptor_lidar.hpp"

#include <QByteArray>
#include <QObject>
#include <QString>

class QTcpSocket;
struct EInterceptorTestProbe;

class EInterceptor final : public QObject {
    Q_OBJECT

public:
    explicit EInterceptor(QObject* parent = nullptr);
    ~EInterceptor() override = default;

    void connectToRadarStation(const QString& address, int port);
    void initLidar();
    bool isConnected() const;

signals:
    void planeTarget(PlaneTarget target);
    void lidarPlaneTarget(QList<PlaneCoord> targets);

protected:
    void timerEvent(QTimerEvent* event) override;

private slots:
    void connected();
    void disconnected();
    void readServer();

private:
    friend struct EInterceptorTestProbe;
    void parseBytes(const QByteArray& bytes, qlonglong receiveTime);

    QString address_;
    int port_{};
    QTcpSocket* socket_{};
    bool connected_{};
    qlonglong lastReceiveTime_{};
    QByteArray receiveBuffer_;
    quint8 expectedLength_{};
    quint8 messageType_{};
    InterceptorLidar* lidar_{};
};
