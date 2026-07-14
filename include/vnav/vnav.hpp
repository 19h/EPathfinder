#pragma once

#include <QGeoCoordinate>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QUdpSocket>
#include <QVector>

#include <cstdint>

struct VehicleTarget {
    std::uint8_t type{};
    std::uint8_t reserved[3]{};
    float elevation{};
    float azimuth{};
};

static_assert(sizeof(VehicleTarget) == 12U);
Q_DECLARE_METATYPE(VehicleTarget)
Q_DECLARE_METATYPE(QList<VehicleTarget>)

class VNav : public QObject {
    Q_OBJECT

public:
    explicit VNav(std::uint16_t receivePort,
                  std::uint16_t transmitPort,
                  QObject* parent = nullptr);

signals:
    void currentPos(qlonglong time, int lat, int lon, float conf);
    void statusChanged(bool avtive);
    void vehicleDetected(qlonglong time, QList<VehicleTarget> targets);
    void checkPlanResult(bool validVNav, bool validVNav2);
    void imageTarget(qlonglong time, short x, short y, uchar type);
    void ahrs(qlonglong time,
              int lat,
              int lon,
              uint alt,
              float roll,
              float pitch,
              float yaw,
              uchar status);
    void yawOffset(qlonglong time, float offset);

public slots:
    void start();
    void stop();
    void reset();
    void setOdoEnabled(bool value);
    void sendCurrentIMU(int lat,
                        int lon,
                        int alt,
                        float yaw,
                        short vx,
                        short vy,
                        float gmblRoll,
                        float gmblPitch,
                        float gmblYaw,
                        float zoom,
                        int gpsLat,
                        int gpsLon);
    void setMode(uchar mode, int trgLat, int trgLon);
    void checkPlan(const QList<QGeoCoordinate>& coords);
    void setRoadPoints(qlonglong time, const QList<QPoint>& coords);

protected:
    void timerEvent(QTimerEvent* event) override;

private slots:
    void readPendingDatagrams();

private:
    struct IMUItem {
        qlonglong time{};
        float yaw{};
        std::uint32_t reserved{};
    };

    static_assert(sizeof(IMUItem) == 16U);

    void sendDatagram(const QByteArray& datagram);
    void checkPlanItem(int aLat, int aLon, int bLat, int bLon);
    void checkNextPlanItem();
    void calcYawOffset(qlonglong time, float yaw);
    int previousIndex(int index) const;
    int nextIndex(int index) const;
    void parseDatagram(const QByteArray& datagram, qlonglong batchTime);

    std::uint16_t receivePort_{};
    std::uint16_t transmitPort_{};
    bool active_{};
    bool odoEnabled_{};
    QUdpSocket* receiveSocket_{};
    QUdpSocket* transmitSocket_{};
    qlonglong lastResetTime_{};
    qlonglong lastStatusPacketTime_{};
    bool statusPacketActive_{};
    bool emittedStatus_{};
    QList<QGeoCoordinate> planCoordinates_;
    qlonglong lastPlanCheckTime_{};
    std::int32_t validVNav2Count_{};
    std::int32_t invalidVNav2Count_{};
    QVector<IMUItem> imuItems_;
    std::int32_t currentImuIndex_{-1};
    bool imuInitialized_{};
    std::int32_t imuCapacity_{512};
    qlonglong interpolationThresholdMs_{2};
};

