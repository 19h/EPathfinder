#pragma once

#include "link/abstract_link.hpp"

#include <QSerialPort>

class QTimerEvent;

class SerialLink final : public AbstractLink {
    Q_OBJECT
    Q_PROPERTY(QString portName READ portName WRITE setPortName NOTIFY portNameChanged)
    Q_PROPERTY(int baudRate READ baudRate WRITE setBaudRate NOTIFY baudRateChanged)

public:
    SerialLink(const QString& portName,
               int baudRate,
               QObject* parent = nullptr,
               unsigned char scanRange = 0);
    ~SerialLink() override;

    bool isUp() const override;
    QString portName() const;
    int baudRate() const;

signals:
    void portNameChanged(QString portName);
    void baudRateChanged(int baudRate);

public slots:
    void up() override;
    void down() override;
    void reconnect() override;
    void sendData(const QByteArray& data) override;
    void sendData(const char* data, qlonglong length) override;
    void setPortName(QString portName);
    void setBaudRate(int baudRate);

protected:
    void timerEvent(QTimerEvent* event) override;
    void switchLink() override;

private slots:
    void readSerialData();
    void portError(QSerialPort::SerialPortError error);

private:
    void connectPort();

    QSerialPort* serialPort_;
    QString initialPortName_;
    int initialBaudRate_;
    qint64 lastErrorMs_ = 0;
    unsigned char consecutiveErrorCount_ = 0;
    unsigned char scanRange_ = 0;
    qint64 lastReadMs_ = 0;
};
