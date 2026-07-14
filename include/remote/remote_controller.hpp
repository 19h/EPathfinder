#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

class QTcpSocket;
struct RemoteControllerTestProbe;

class RemoteController final : public QObject {
    Q_OBJECT

public:
    explicit RemoteController(QObject* parent = nullptr);
    ~RemoteController() override = default;

    void connectToControllerServer(const QString& address, int port);
    bool isConnected() const;

signals:
    void enableControl();
    void disableControl();
    void controlValues(int roll, int pitch, int throttle);

private slots:
    void sendMsgIAmPlane();
    void connected();
    void disconnected();
    void readServer();

private:
    friend struct RemoteControllerTestProbe;
    void parseBytes(const QByteArray& bytes, qlonglong receiveTime);

    QString address_;
    int port_{};
    QTcpSocket* socket_{};
    bool connected_{};
    qlonglong lastReceiveTime_{};
    QByteArray receiveBuffer_;
    quint8 expectedLength_{};
    quint8 messageType_{};
};
