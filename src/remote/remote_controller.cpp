#include "remote/remote_controller.hpp"

#include <QDateTime>
#include <QTcpSocket>
#include <QtEndian>

RemoteController::RemoteController(QObject* parent)
    : QObject(parent)
{
}

bool RemoteController::isConnected() const { return connected_; }

void RemoteController::connectToControllerServer(const QString& address,
                                                  int port)
{
    if (socket_) {
        socket_->deleteLater();
    }
    address_ = address;
    port_ = port;
    socket_ = new QTcpSocket(this);
    connect(socket_, &QTcpSocket::connected,
            this, &RemoteController::connected);
    connect(socket_, &QTcpSocket::disconnected,
            this, &RemoteController::disconnected);
    connect(socket_, &QTcpSocket::readyRead,
            this, &RemoteController::readServer);
    socket_->connectToHost(address_, static_cast<quint16>(port_));
}

void RemoteController::sendMsgIAmPlane()
{
    if (socket_) {
        socket_->write(QByteArray::fromHex("e00eaa"));
    }
}

void RemoteController::connected()
{
    connected_ = true;
    sendMsgIAmPlane();
}

void RemoteController::disconnected() { connected_ = false; }

void RemoteController::readServer()
{
    if (!socket_) {
        return;
    }
    const qlonglong receiveTime = QDateTime::currentMSecsSinceEpoch();
    parseBytes(socket_->readAll(), receiveTime);
}

void RemoteController::parseBytes(const QByteArray& bytes,
                                  qlonglong receiveTime)
{
    if (receiveTime - lastReceiveTime_ > 10000 && !receiveBuffer_.isEmpty()) {
        receiveBuffer_.clear();
    }
    lastReceiveTime_ = receiveTime;

    for (const char rawByte : bytes) {
        const quint8 byte = static_cast<quint8>(rawByte);
        switch (receiveBuffer_.size()) {
        case 0:
            if (byte == 0xe0U) {
                receiveBuffer_.append(rawByte);
            }
            break;
        case 1:
            if (byte == 0x0eU) {
                receiveBuffer_.append(rawByte);
            } else {
                receiveBuffer_.clear();
            }
            break;
        case 2:
            receiveBuffer_.append(rawByte);
            messageType_ = byte;
            if (messageType_ == 1U) {
                emit enableControl();
                receiveBuffer_.clear();
            } else if (messageType_ == 2U) {
                emit disableControl();
                receiveBuffer_.clear();
            } else if (messageType_ == 3U) {
                expectedLength_ = 9U;
            } else {
                receiveBuffer_.clear();
            }
            break;
        default:
            receiveBuffer_.append(rawByte);
            if (receiveBuffer_.size() == expectedLength_) {
                if (messageType_ == 3U) {
                    const auto* data = reinterpret_cast<const uchar*>(
                        receiveBuffer_.constData());
                    emit controlValues(qFromLittleEndian<quint16>(data + 3),
                                       qFromLittleEndian<quint16>(data + 5),
                                       qFromLittleEndian<quint16>(data + 7));
                }
                receiveBuffer_.clear();
            }
            break;
        }
    }
}

