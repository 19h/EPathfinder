#include "link/tcp_link.hpp"

#include <QDebug>

TcpLink::TcpLink(const QString& address,
                 const quint16 port,
                 QObject* const parent)
    : AbstractLink(parent)
    , address_(address)
    , port_(port)
    , socket_(new QTcpSocket(this))
{
    connect(socket_, &QTcpSocket::connected, this, &TcpLink::connected);
    connect(socket_, &QTcpSocket::disconnected, this, &TcpLink::disconnected);
    connect(socket_, &QTcpSocket::readyRead, this, &TcpLink::readData);
}

bool TcpLink::isUp() const
{
    return isUp_;
}

void TcpLink::up()
{
    if (!isUp()) {
        socket_->connectToHost(address_,
                               port_,
                               QIODevice::ReadWrite,
                               QAbstractSocket::AnyIPProtocol);
    }
}

void TcpLink::down()
{
    socket_->disconnectFromHost();
}

void TcpLink::sendData(const QByteArray& data)
{
    socket_->write(data);
    socket_->flush();
}

void TcpLink::sendData(const char* const data, const qlonglong length)
{
    sendData(QByteArray(data, static_cast<int>(length)));
}

void TcpLink::connected()
{
    isUp_ = true;
    qDebug() << address_ << ':' << port_ << "connected";
}

void TcpLink::disconnected()
{
    isUp_ = false;
    qDebug() << address_ << ':' << port_ << "disconnected";
}

void TcpLink::readData()
{
    emit dataReceived(socket_->readAll());
}
