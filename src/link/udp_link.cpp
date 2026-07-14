#include "link/udp_link.hpp"

#include <QDebug>
#include <QHostAddress>

UdpLink::UdpLink(const int rxPort,
                 const QString& address,
                 const int txPort,
                 QObject* const parent)
    : AbstractLink(parent)
    , socket_(new QUdpSocket(this))
    , rxPort_(rxPort)
    , address_(address)
    , txPort_(txPort)
{
    connect(socket_, &QUdpSocket::readyRead, this, &UdpLink::readPendingDatagrams);
}

bool UdpLink::isUp() const
{
    return socket_->state() == QAbstractSocket::BoundState;
}

int UdpLink::rxPort() const noexcept
{
    return rxPort_;
}

QString UdpLink::address() const
{
    return address_;
}

int UdpLink::txPort() const noexcept
{
    return txPort_;
}

void UdpLink::up()
{
    if (isUp()) {
        return;
    }
    if (socket_->bind(static_cast<quint16>(rxPort_))) {
        emit upChanged(true);
    } else {
        qWarning("UDP connection error: '%s'!",
                 socket_->errorString().toLocal8Bit().constData());
        socket_->close();
    }
}

void UdpLink::down()
{
    if (isUp()) {
        socket_->close();
        emit upChanged(false);
    }
}

void UdpLink::sendData(const QByteArray& data)
{
    socket_->writeDatagram(data,
                           QHostAddress(address_),
                           static_cast<quint16>(txPort_));
}

void UdpLink::sendData(const char* const data, const qlonglong length)
{
    sendData(QByteArray(data, static_cast<int>(length)));
}

void UdpLink::setRxPort(const int port)
{
    if (rxPort_ == port) {
        return;
    }
    rxPort_ = port;
    if (isUp()) {
        down();
        up();
    }
    emit rxPortChanged(port);
}

void UdpLink::setAddress(const QString& address)
{
    if (address_ != address) {
        address_ = address;
        emit addressChanged(address);
    }
}

void UdpLink::setTxPort(const int port)
{
    if (txPort_ != port) {
        txPort_ = port;
        emit txPortChanged(port);
    }
}

void UdpLink::readPendingDatagrams()
{
    while (socket_->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(socket_->pendingDatagramSize()));
        socket_->readDatagram(datagram.data(), datagram.size(), nullptr, nullptr);
        emit dataReceived(datagram);
    }
}
