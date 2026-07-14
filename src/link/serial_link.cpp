#include "link/serial_link.hpp"

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QTimerEvent>

SerialLink::SerialLink(const QString& portName,
                       const int baudRate,
                       QObject* const parent,
                       const unsigned char scanRange)
    : AbstractLink(parent)
    , serialPort_(new QSerialPort(portName, this))
    , initialPortName_(portName)
    , initialBaudRate_(baudRate)
    , scanRange_(scanRange)
{
    serialPort_->setBaudRate(baudRate);
    qDebug() << "init serial port:" << portName << " baudRate:" << baudRate
             << " rc:" << static_cast<bool>(scanRange);
    linkName_ = portName;
    connectPort();
    startTimer(50, Qt::CoarseTimer);
}

SerialLink::~SerialLink()
{
    qDebug() << "close port " << serialPort_->portName();
    serialPort_->close();
}

bool SerialLink::isUp() const
{
    return serialPort_->error() == QSerialPort::NoError && serialPort_->isOpen();
}

void SerialLink::connectPort()
{
    connect(serialPort_, &QSerialPort::readyRead,
            this, &SerialLink::readSerialData);
    connect(serialPort_, &QSerialPort::errorOccurred,
            this, &SerialLink::portError);
}

void SerialLink::switchLink()
{
    QString nextPort = serialPort_->portName();
    bool ok = false;
    const int suffix = nextPort.right(1).toInt(&ok, 10);
    if (!ok) {
        return;
    }

    serialPort_->disconnect();
    serialPort_->deleteLater();
    nextPort.replace(nextPort.size() - 1,
                     1,
                     QString::number((suffix + 1) % 5, 10));
    qDebug() << "reconnect to serial port:" << nextPort
             << " baudRate:" << initialBaudRate_;
    serialPort_ = new QSerialPort(nextPort, this);
    serialPort_->setBaudRate(initialBaudRate_);
    connectPort();
}

QString SerialLink::portName() const
{
    return serialPort_->portName();
}

int SerialLink::baudRate() const
{
    return serialPort_->baudRate();
}

void SerialLink::up()
{
    if (isUp()) {
        return;
    }

    if (scanRange_ != 0) {
        bool ok = false;
        const int suffix = initialPortName_.right(1).toInt(&ok, 10);
        if (ok) {
            QString prefix = initialPortName_;
            prefix.chop(1);
            for (int offset = 0; offset <= scanRange_; ++offset) {
                const QString candidate = prefix + QString::number(suffix + offset, 10);
                if (QFile::exists(candidate)) {
                    qDebug() << "switch serial link to:" << candidate;
                    serialPort_->setPortName(candidate);
                    break;
                }
            }
        }
    }

    if (!serialPort_->open(QIODevice::ReadWrite)) {
        qWarning().noquote()
            << QStringLiteral("Serial port '%1' connection error: '%2'!")
                   .arg(initialPortName_, serialPort_->errorString());
        serialPort_->close();
        return;
    }

    qDebug() << portName() << "connected";
    emit upChanged(true);
}

void SerialLink::down()
{
    if (!isUp()) {
        return;
    }
    serialPort_->close();
    qDebug() << portName() << "disconnected";
    emit upChanged(false);
}

void SerialLink::sendData(const QByteArray& data)
{
    serialPort_->write(data);
}

void SerialLink::sendData(const char* const data, const qlonglong length)
{
    serialPort_->write(data, length);
}

void SerialLink::setPortName(const QString portName)
{
    if (serialPort_->portName() != portName) {
        serialPort_->setPortName(portName);
        emit portNameChanged(serialPort_->portName());
    }
}

void SerialLink::setBaudRate(const int baudRate)
{
    if (serialPort_->baudRate() != baudRate) {
        serialPort_->setBaudRate(baudRate);
        emit baudRateChanged(serialPort_->baudRate());
    }
}

void SerialLink::readSerialData()
{
    lastReadMs_ = QDateTime::currentMSecsSinceEpoch();
    const QByteArray data = serialPort_->readAll();
    if (!data.isEmpty()) {
        emit dataReceived(data);
    }
}

void SerialLink::timerEvent(QTimerEvent* const event)
{
    Q_UNUSED(event);
    if (serialPort_ != nullptr
        && QDateTime::currentMSecsSinceEpoch() - lastReadMs_ > 100) {
        readSerialData();
    }
}

void SerialLink::portError(const QSerialPort::SerialPortError error)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastErrorMs_ <= 100) {
        ++consecutiveErrorCount_;
        if (consecutiveErrorCount_ > 2) {
            qDebug() << portName() << " errors... disconnect signal";
            disconnect(serialPort_, &QSerialPort::errorOccurred,
                       this, &SerialLink::portError);
            QTimer::singleShot(2000, this, &SerialLink::reconnect);
        }
        return;
    }

    qDebug() << "serial port error code:" << error
             << " error:" << serialPort_->errorString();
    lastErrorMs_ = now;
    QTimer::singleShot(100, this, &SerialLink::up);
}

void SerialLink::reconnect()
{
    qDebug() << portName() << " reconnect";
    consecutiveErrorCount_ = 0;
    serialPort_->disconnect();
    serialPort_->deleteLater();
    serialPort_ = new QSerialPort(initialPortName_, this);
    serialPort_->setBaudRate(initialBaudRate_);
    connectPort();
}
