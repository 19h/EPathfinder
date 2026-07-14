#include "interception/einterceptor.hpp"

#ifdef EPATHFINDER_HAS_LIVOX_SDK
#include "lidar/interceptor_livox.hpp"
#endif

#include <QDateTime>
#include <QTcpSocket>
#include <QTimer>
#include <QTimerEvent>
#include <QtEndian>

#include <cstring>

namespace {

template <typename T>
T littleEndianValue(const QByteArray& bytes, int offset)
{
    if (offset < 0 || bytes.size() - offset < static_cast<int>(sizeof(T))) {
        return {};
    }
    T value{};
    std::memcpy(&value, bytes.constData() + offset, sizeof(T));
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    value = qFromLittleEndian(value);
#endif
    return value;
}

} // namespace

EInterceptor::EInterceptor(QObject* parent)
    : QObject(parent)
{
    startTimer(1000);
}

bool EInterceptor::isConnected() const { return connected_; }

void EInterceptor::connectToRadarStation(const QString& address, int port)
{
    if (!socket_) {
        socket_ = new QTcpSocket(this);
        connect(socket_, &QTcpSocket::connected,
                this, &EInterceptor::connected);
        connect(socket_, &QTcpSocket::disconnected,
                this, &EInterceptor::disconnected);
        connect(socket_, &QTcpSocket::readyRead,
                this, &EInterceptor::readServer);
    } else {
        socket_->abort();
    }
    address_ = address;
    port_ = port;
    socket_->connectToHost(address_, static_cast<quint16>(port_));
}

void EInterceptor::timerEvent(QTimerEvent* event)
{
    Q_UNUSED(event)
    if (socket_ && !address_.isEmpty() && !connected_) {
        socket_->abort();
        socket_->connectToHost(address_, static_cast<quint16>(port_));
    }
}

void EInterceptor::connected() { connected_ = true; }
void EInterceptor::disconnected() { connected_ = false; }

void EInterceptor::readServer()
{
    if (!socket_) {
        return;
    }
    const qlonglong receiveTime = QDateTime::currentMSecsSinceEpoch();
    parseBytes(socket_->readAll(), receiveTime);
}

void EInterceptor::parseBytes(const QByteArray& bytes,
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
            if (messageType_ == 0x13U) {
                expectedLength_ = 0x23U;
            } else {
                receiveBuffer_.clear();
            }
            break;
        default:
            receiveBuffer_.append(rawByte);
            if (receiveBuffer_.size() == expectedLength_) {
                PlaneTarget target;
                target.time = receiveTime - 200;
                const int lat = littleEndianValue<qint32>(receiveBuffer_, 13);
                const int lon = littleEndianValue<qint32>(receiveBuffer_, 17);
                const int alt = littleEndianValue<qint32>(receiveBuffer_, 21);
                target.coordinate = QGeoCoordinate(lat / 1.0e7,
                                                   lon / 1.0e7,
                                                   alt / 1000.0);
                target.azimuth = littleEndianValue<float>(receiveBuffer_, 25);
                target.groundSpeed =
                    littleEndianValue<float>(receiveBuffer_, 29);
                target.valid = false;
                emit planeTarget(target);
                receiveBuffer_.clear();
            }
            break;
        }
    }
}

void EInterceptor::initLidar()
{
#ifdef EPATHFINDER_HAS_LIVOX_SDK
    if (lidar_) {
        return;
    }
    auto* const livox = new InterceptorLivox(this);
    lidar_ = livox;
    connect(livox, &InterceptorLidar::lidarData,
            this, &EInterceptor::lidarPlaneTarget);
    QTimer::singleShot(1, livox, &InterceptorLivox::Init);
#endif
}
