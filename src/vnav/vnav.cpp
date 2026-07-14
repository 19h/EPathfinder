#include "vnav/vnav.hpp"

#include <QDateTime>
#include <QHostAddress>
#include <QTimerEvent>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <type_traits>

namespace {

constexpr char kPrefix0 = static_cast<char>(0xe0);
constexpr char kPrefix1 = static_cast<char>(0x0e);

template <typename T>
void appendLittleEndian(QByteArray& output, const T value)
{
    static_assert(std::is_trivially_copyable_v<T>);
    const auto* bytes = reinterpret_cast<const char*>(&value);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    output.append(bytes, static_cast<qsizetype>(sizeof(T)));
#else
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        output.append(bytes[sizeof(T) - i - 1U]);
    }
#endif
}

template <typename T>
bool readLittleEndian(const QByteArray& input,
                      const qsizetype offset,
                      T& value)
{
    static_assert(std::is_trivially_copyable_v<T>);
    if (offset < 0
        || input.size() - offset < static_cast<qsizetype>(sizeof(T))) {
        return false;
    }
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    std::memcpy(&value, input.constData() + offset, sizeof(T));
#else
    char bytes[sizeof(T)];
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        bytes[i] = input[offset + sizeof(T) - i - 1U];
    }
    std::memcpy(&value, bytes, sizeof(T));
#endif
    return true;
}

QByteArray command(const std::uint8_t id)
{
    QByteArray result;
    result.reserve(3);
    result.append(kPrefix0);
    result.append(kPrefix1);
    result.append(static_cast<char>(id));
    return result;
}

float normalize360(float angle)
{
    if (angle < 0.0F) {
        angle += 360.0F;
    } else if (angle >= 360.0F) {
        angle -= 360.0F;
    }
    return angle;
}

} // namespace

VNav::VNav(const std::uint16_t receivePort,
           const std::uint16_t transmitPort,
           QObject* const parent)
    : QObject(parent)
    , receivePort_(receivePort)
    , transmitPort_(transmitPort)
    , receiveSocket_(new QUdpSocket(this))
    , transmitSocket_(new QUdpSocket(this))
{
    (void)receiveSocket_->bind(receivePort_);
    connect(receiveSocket_,
            &QUdpSocket::readyRead,
            this,
            &VNav::readPendingDatagrams);
    imuItems_.resize(imuCapacity_);
    (void)startTimer(500, Qt::CoarseTimer);
}

void VNav::sendDatagram(const QByteArray& datagram)
{
    (void)transmitSocket_->writeDatagram(datagram,
                                         QHostAddress::LocalHost,
                                         transmitPort_);
}

void VNav::start()
{
    if (active_) {
        return;
    }
    active_ = true;
    sendDatagram(command(1U));
}

void VNav::stop()
{
    if (!active_) {
        return;
    }
    active_ = false;
    sendDatagram(command(0U));
}

void VNav::reset()
{
    if (!active_) {
        return;
    }
    const qlonglong now = QDateTime::currentMSecsSinceEpoch();
    if (lastResetTime_ == now) {
        return;
    }
    lastResetTime_ = now;
    sendDatagram(command(2U));
}

void VNav::setOdoEnabled(const bool value)
{
    odoEnabled_ = value;
}

void VNav::setMode(const uchar mode, const int targetLat, const int targetLon)
{
    if (!active_ && mode != 0U && mode != 4U && mode != 5U) {
        return;
    }
    QByteArray datagram = command(4U);
    datagram.append(static_cast<char>(mode));
    if (mode == 1U) {
        appendLittleEndian(datagram, targetLat);
        appendLittleEndian(datagram, targetLon);
    } else if (mode > 5U) {
        return;
    }
    sendDatagram(datagram);
}

void VNav::setRoadPoints(const qlonglong time,
                         const QList<QPoint>& coordinates)
{
    QByteArray datagram = command(7U);
    appendLittleEndian(datagram, time);
    for (const QPoint& coordinate : coordinates) {
        appendLittleEndian(
            datagram, static_cast<std::int16_t>(coordinate.x()));
        appendLittleEndian(
            datagram, static_cast<std::int16_t>(coordinate.y()));
    }
    sendDatagram(datagram);
}

void VNav::sendCurrentIMU(const int lat,
                          const int lon,
                          const int alt,
                          const float yaw,
                          const short vx,
                          const short vy,
                          const float gimbalRoll,
                          const float gimbalPitch,
                          const float gimbalYaw,
                          const float zoom,
                          const int gpsLat,
                          const int gpsLon)
{
    const qlonglong time = QDateTime::currentMSecsSinceEpoch() - 1;
    const float cameraYaw = normalize360(yaw + gimbalYaw);

    QByteArray datagram = command(3U);
    appendLittleEndian(datagram, time);
    appendLittleEndian(datagram, lat);
    appendLittleEndian(datagram, lon);
    appendLittleEndian(datagram, alt);
    appendLittleEndian(datagram, static_cast<std::int16_t>(vx));
    appendLittleEndian(datagram, static_cast<std::int16_t>(vy));
    appendLittleEndian(datagram, gimbalRoll);
    appendLittleEndian(datagram, gimbalPitch);
    appendLittleEndian(datagram, cameraYaw);
    appendLittleEndian(datagram, zoom);
    appendLittleEndian(datagram, gpsLat);
    appendLittleEndian(datagram, gpsLon);
    sendDatagram(datagram);

    ++currentImuIndex_;
    if (imuInitialized_) {
        if (currentImuIndex_ == imuCapacity_) {
            currentImuIndex_ = 0;
        }
    } else if (currentImuIndex_ == 1) {
        imuInitialized_ = true;
    }
    imuItems_[currentImuIndex_] = IMUItem{time, cameraYaw, 0U};
}

int VNav::previousIndex(int index) const
{
    if (index <= 0) {
        index = imuCapacity_;
    }
    return index - 1;
}

int VNav::nextIndex(const int index) const
{
    const int next = index + 1;
    return next >= imuCapacity_ ? 0 : next;
}

void VNav::calcYawOffset(const qlonglong time, const float yaw)
{
    if (!imuInitialized_) {
        return;
    }

    int index = currentImuIndex_;
    IMUItem before = imuItems_[index];
    qlonglong delta = time - before.time;
    if (delta < 0) {
        const int oldest = imuInitialized_
            ? nextIndex(currentImuIndex_)
            : 0;
        while (index != oldest) {
            index = previousIndex(index);
            const IMUItem candidate = imuItems_[index];
            if (candidate.time == 0) {
                break;
            }
            before = candidate;
            delta = time - before.time;
            if (delta >= 0) {
                break;
            }
        }
        if (delta < 0) {
            delta = 0;
        }
    }

    float imuYaw = before.yaw;
    if (delta >= interpolationThresholdMs_
        && currentImuIndex_ != index) {
        const IMUItem after = imuItems_[nextIndex(index)];
        const qlonglong duration = after.time - before.time;
        if (duration != 0) {
            float difference = after.yaw - before.yaw;
            if (difference < 0.0F) {
                difference += 360.0F;
            }
            const float ratio = static_cast<float>(delta)
                / static_cast<float>(duration);
            if (difference <= 180.0F) {
                imuYaw = before.yaw + ratio * difference;
            } else {
                imuYaw = before.yaw - ratio * (360.0F - difference);
            }
            imuYaw = normalize360(imuYaw);
        }
    }

    float offset = yaw - imuYaw;
    if (offset < 0.0F) {
        offset += 360.0F;
    }
    if (offset > 180.0F) {
        offset -= 360.0F;
    }
    emit yawOffset(time, offset);
}

void VNav::checkPlanItem(const int aLat,
                         const int aLon,
                         const int bLat,
                         const int bLon)
{
    lastPlanCheckTime_ = QDateTime::currentMSecsSinceEpoch();
    QByteArray datagram = command(6U);
    appendLittleEndian(datagram, aLat);
    appendLittleEndian(datagram, aLon);
    appendLittleEndian(datagram, bLat);
    appendLittleEndian(datagram, bLon);
    sendDatagram(datagram);
}

void VNav::checkNextPlanItem()
{
    if (planCoordinates_.size() <= 1) {
        return;
    }
    const QGeoCoordinate& first = planCoordinates_[0];
    const QGeoCoordinate& second = planCoordinates_[1];
    checkPlanItem(static_cast<int>(first.latitude() * 10000000.0),
                  static_cast<int>(first.longitude() * 10000000.0),
                  static_cast<int>(second.latitude() * 10000000.0),
                  static_cast<int>(second.longitude() * 10000000.0));
}

void VNav::checkPlan(const QList<QGeoCoordinate>& coordinates)
{
    planCoordinates_ = coordinates;
    validVNav2Count_ = 0;
    invalidVNav2Count_ = 0;
    if (QDateTime::currentMSecsSinceEpoch() - lastPlanCheckTime_ > 10000
        && planCoordinates_.size() > 1) {
        checkNextPlanItem();
    }
}

void VNav::timerEvent(QTimerEvent* const event)
{
    Q_UNUSED(event)
    const qlonglong now = QDateTime::currentMSecsSinceEpoch();
    bool currentStatus = statusPacketActive_;
    if (currentStatus) {
        currentStatus = now - lastStatusPacketTime_ <= 1999;
    }
    if (currentStatus != emittedStatus_) {
        emittedStatus_ = currentStatus;
        emit statusChanged(currentStatus);
    }

    if (now - lastPlanCheckTime_ > 10000
        && planCoordinates_.size() > 1) {
        checkNextPlanItem();
    }
}

void VNav::readPendingDatagrams()
{
    const qlonglong batchTime = QDateTime::currentMSecsSinceEpoch();
    while (receiveSocket_->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(
            static_cast<qsizetype>(receiveSocket_->pendingDatagramSize()));
        (void)receiveSocket_->readDatagram(datagram.data(), datagram.size());
        parseDatagram(datagram, batchTime);
    }
}

void VNav::parseDatagram(const QByteArray& datagram,
                         const qlonglong batchTime)
{
    if (datagram.size() < 3 || datagram[0] != kPrefix0
        || datagram[1] != kPrefix1) {
        return;
    }
    const std::uint8_t id = static_cast<std::uint8_t>(datagram[2]);

    if (id == 0U) {
        if (datagram.size() < 4) {
            return;
        }
        statusPacketActive_ = datagram[3] == 0;
        lastStatusPacketTime_ = QDateTime::currentMSecsSinceEpoch();
        return;
    }

    if (id == 1U) {
        qlonglong time = QDateTime::currentMSecsSinceEpoch();
        int lat = 0;
        int lon = 0;
        std::uint8_t confidence = 0U;
        if (datagram.size() <= 12) {
            if (!readLittleEndian(datagram, 3, lat)
                || !readLittleEndian(datagram, 7, lon)
                || datagram.size() < 12) {
                return;
            }
            confidence = static_cast<std::uint8_t>(datagram[11]);
        } else {
            if (!readLittleEndian(datagram, 3, time)
                || !readLittleEndian(datagram, 11, lat)
                || !readLittleEndian(datagram, 15, lon)
                || datagram.size() < 20) {
                return;
            }
            confidence = static_cast<std::uint8_t>(datagram[19]);
        }
        emit currentPos(time,
                        lat,
                        lon,
                        static_cast<float>(confidence) / 255.0F);
        return;
    }

    if (id == 2U) {
        int lat = 0;
        int lon = 0;
        if (!readLittleEndian(datagram, 3, lat)
            || !readLittleEndian(datagram, 7, lon)
            || datagram.size() < 12) {
            return;
        }
        if (odoEnabled_) {
            emit currentPos(QDateTime::currentMSecsSinceEpoch(),
                            lat,
                            lon,
                            1.0F);
        }
        return;
    }

    if (id == 3U || id == 4U || id == 7U) {
        return;
    }

    if (id == 5U) {
        std::uint16_t count = 0U;
        if (!readLittleEndian(datagram, 3, count)
            || datagram.size() < 5 + static_cast<qsizetype>(count) * 9) {
            return;
        }
        QList<VehicleTarget> targets;
        for (std::uint16_t i = 0; i < count; ++i) {
            const qsizetype offset = 5 + static_cast<qsizetype>(i) * 9;
            VehicleTarget target;
            target.type = static_cast<std::uint8_t>(datagram[offset]);
            (void)readLittleEndian(datagram, offset + 1, target.elevation);
            (void)readLittleEndian(datagram, offset + 5, target.azimuth);
            targets.append(target);
        }
        if (count != 0U) {
            emit vehicleDetected(batchTime, targets);
        }
        return;
    }

    if (id == 6U) {
        int aLat = 0;
        int aLon = 0;
        int bLat = 0;
        int bLon = 0;
        if (!readLittleEndian(datagram, 3, aLat)
            || !readLittleEndian(datagram, 7, aLon)
            || !readLittleEndian(datagram, 11, bLat)
            || !readLittleEndian(datagram, 15, bLon)
            || datagram.size() < 20) {
            return;
        }
        if (datagram[19] != 0) {
            emit checkPlanResult(false, false);
            planCoordinates_.clear();
            return;
        }
        if (datagram.size() > 20) {
            if (datagram[20] != 0) {
                ++invalidVNav2Count_;
            } else {
                ++validVNav2Count_;
            }
        }
        if (planCoordinates_.size() <= 1) {
            return;
        }
        const QGeoCoordinate& first = planCoordinates_[0];
        const QGeoCoordinate& second = planCoordinates_[1];
        const bool matches =
            static_cast<int>(first.latitude() * 10000000.0) == aLat
            && static_cast<int>(first.longitude() * 10000000.0) == aLon
            && static_cast<int>(second.latitude() * 10000000.0) == bLat
            && static_cast<int>(second.longitude() * 10000000.0) == bLon;
        if (matches) {
            planCoordinates_.removeFirst();
        }
        if (planCoordinates_.size() > 1) {
            checkNextPlanItem();
        } else {
            const bool validVNav2 = validVNav2Count_ > 0
                && invalidVNav2Count_ == 0;
            emit checkPlanResult(true, validVNav2);
            planCoordinates_.clear();
        }
        return;
    }

    if (id == 8U) {
        qlonglong time = 0;
        std::int16_t x = 0;
        std::int16_t y = 0;
        if (!readLittleEndian(datagram, 3, time)
            || !readLittleEndian(datagram, 11, x)
            || !readLittleEndian(datagram, 13, y)
            || datagram.size() < 16) {
            return;
        }
        emit imageTarget(time,
                         x,
                         y,
                         static_cast<uchar>(datagram[15]));
        return;
    }

    if (id == 9U) {
        qlonglong time = 0;
        std::int16_t x = 0;
        std::int16_t y = 0;
        if (!readLittleEndian(datagram, 3, time)
            || !readLittleEndian(datagram, 11, x)
            || !readLittleEndian(datagram, 13, y)) {
            return;
        }
        emit imageTarget(time, x, y, 2U);
        return;
    }

    if (id == 10U) {
        qlonglong time = 0;
        int lat = 0;
        int lon = 0;
        std::uint32_t alt = 0U;
        float roll = 0.0F;
        float pitch = 0.0F;
        float yaw = 0.0F;
        if (!readLittleEndian(datagram, 3, time)
            || !readLittleEndian(datagram, 11, lat)
            || !readLittleEndian(datagram, 15, lon)
            || !readLittleEndian(datagram, 19, alt)
            || !readLittleEndian(datagram, 23, roll)
            || !readLittleEndian(datagram, 27, pitch)
            || !readLittleEndian(datagram, 31, yaw)
            || datagram.size() < 36) {
            return;
        }
        const uchar status = static_cast<uchar>(datagram[35]);
        emit ahrs(time, lat, lon, alt, roll, pitch, yaw, status);
        calcYawOffset(time, yaw);
    }
}

