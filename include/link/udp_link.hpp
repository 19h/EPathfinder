#pragma once

#include "link/abstract_link.hpp"

#include <QUdpSocket>

class UdpLink final : public AbstractLink {
    Q_OBJECT
    Q_PROPERTY(int rxPort READ rxPort WRITE setRxPort NOTIFY rxPortChanged)
    Q_PROPERTY(QString address READ address WRITE setAddress NOTIFY addressChanged)
    Q_PROPERTY(int txPort READ txPort WRITE setTxPort NOTIFY txPortChanged)

public:
    UdpLink(int rxPort,
            const QString& address,
            int txPort,
            QObject* parent = nullptr);

    bool isUp() const override;
    int rxPort() const noexcept;
    QString address() const;
    int txPort() const noexcept;

signals:
    void rxPortChanged(int port);
    void addressChanged(QString address);
    void txPortChanged(int port);

public slots:
    void up() override;
    void down() override;
    void sendData(const QByteArray& data) override;
    void sendData(const char* data, qlonglong length) override;
    void setRxPort(int port);
    void setAddress(const QString& address);
    void setTxPort(int port);

private slots:
    void readPendingDatagrams();

private:
    QUdpSocket* socket_;
    int rxPort_;
    QString address_;
    int txPort_;
};
