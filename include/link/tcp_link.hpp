#pragma once

#include "link/abstract_link.hpp"

#include <QTcpSocket>

class TcpLink final : public AbstractLink {
    Q_OBJECT

public:
    TcpLink(const QString& address, quint16 port, QObject* parent = nullptr);

    bool isUp() const override;

public slots:
    void up() override;
    void down() override;
    void sendData(const QByteArray& data) override;
    void sendData(const char* data, qlonglong length) override;

private slots:
    void connected();
    void disconnected();
    void readData();

private:
    QString address_;
    quint16 port_;
    QTcpSocket* socket_;
    bool isUp_ = false;
};
