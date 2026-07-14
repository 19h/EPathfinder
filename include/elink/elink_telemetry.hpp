#pragma once

#include "elink/elink_abstract_handler.hpp"
#include "mavlink/plan_handler.hpp"

#include <QGeoCoordinate>
#include <QList>
#include <QProcess>
#include <QString>

#include <cstdint>

class QTimerEvent;
class QUdpSocket;
struct ELinkTelemetryTestProbe;

// Layout recovered from the two planeTarget construction sites at RVAs
// 0x3d49c and 0x39170.  The final coordinate is default constructed by both
// sites; no recovered producer assigns it.
struct PlaneTarget {
    QGeoCoordinate coordinate;
    qlonglong time{};
    float azimuth{};
    float groundSpeed{};
    bool valid{};
    std::uint8_t reserved25[7]{};
    qlonglong sourceTime{};
    qlonglong reservedTime{};
    QGeoCoordinate referenceCoordinate;
};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
static_assert(sizeof(PlaneTarget) == 56U);
#endif

Q_DECLARE_METATYPE(PlaneTarget)

class ELinkTelemetry final : public ELinkAbstractHandler {
    Q_OBJECT

public:
    explicit ELinkTelemetry(ELinkCommunicator* communicator,
                            unsigned char configuration);
    ~ELinkTelemetry() override;

    void setVersion(unsigned char version);
    unsigned short telemetryPlaneId() const;
    void setTelemetryPlaneId(unsigned short planeId);
    QString telemetryPlaneUid() const;
    void setTelemetryPlaneUid(const QString& uid);
    bool isConnected() const;

    void getTelemetryStatus();
    void setTelemetryConfig(unsigned char first, unsigned char second);
    void setHostAddress(const QString& address);
    void Init(const QString& address, unsigned short port);

signals:
    void telemetryConnected();
    void telemetryDisconnected();
    void peekAtGPS();
    void gotoLastWP();
    void selfDestruct();
    void setCurrentPos(qlonglong time, int lat, int lon);
    void resetVNav();
    void setLastWPCoord(int lat, int lon);
    void startSearch(qlonglong time);
    void planeTarget(PlaneTarget target);
    void remoteControlEnable(unsigned char mode,
                             int roll,
                             int pitch,
                             int throttle,
                             unsigned char options);
    void remoteControlDisable();
    void startManuLand();
    void stopManuLand();
    void setInterceptorEnabled(bool enabled);

public slots:
    void processMessage() override;
    void sendPosAtt(qlonglong time,
                    int lat,
                    int lon,
                    int alt,
                    float roll,
                    float pitch,
                    float yaw,
                    float airSpeed);
    void sendGPSPos(qlonglong time, int lat, int lon, int alt);
    void sendTelemetryCameraParams(qlonglong time,
                                   float gimbalRoll,
                                   float gimbalPitch,
                                   float gimbalYaw);
    void sendLastWPPos(qlonglong time, int lat, int lon);
    void sendRadarEmulationInfo(qlonglong time,
                                int lat,
                                int lon,
                                int alt,
                                float azimuth,
                                float groundSpeed);
    void sendPlan(qlonglong time, Plan plan);
    void sendGPSStatus(qlonglong time,
                       unsigned char epStatus,
                       unsigned char gpsStatus,
                       unsigned char satellites);
    void sendCurrentFlightMode(qlonglong time, unsigned char mode);
    void sendCurrentMode(qlonglong time,
                         unsigned char mode,
                         unsigned short targetAltitude);
    void sendPlanModeInfo(qlonglong time,
                          unsigned char nextWaypoint,
                          unsigned short nextWaypointDistance,
                          float deltaAzimuth);
    void sendFindModeInfo(qlonglong time,
                          unsigned char direction,
                          unsigned short centerDistance);
    void sendTelemetryDistanceInfo(qlonglong time,
                                   int battery,
                                   int homeDistance,
                                   int distanceTraveled);
    void sendTargetCorrectionModeInfo(qlonglong time,
                                      int targetLat,
                                      int targetLon,
                                      unsigned short dtpCount,
                                      unsigned short distance,
                                      float deltaAzimuth);
    void sendTargetModeInfo(qlonglong time,
                            int targetLat,
                            int targetLon,
                            unsigned char state,
                            unsigned short dtpCount,
                            unsigned short distance,
                            float deltaAzimuth);
    void sendExplorationInfo(qlonglong time,
                             unsigned short realWindDirection,
                             float realWindSpeed);
    void setLaunchReady(bool ready);
    void setLaunched();
    void startVideoStream();
    void enableFullPower();
    void getVideoStreamStatus();
    void enableVideoStream();
    void disableVideoStream();

protected:
    void timerEvent(QTimerEvent* event) override;

private slots:
    void connectToUDPTelemetry();
    void connectToUDPCommands();
    void connectToUDPEPLaunch();
    void readTelemetryPendingDatagrams();
    void readTelemetryPendingDatagrams2();
    void readModemCommandPendingDatagrams();
    void readEPLaunchDatagrams();
    void pingTelemetry();
    void pingTelemetryFinished(int exitCode,
                               QProcess::ExitStatus exitStatus);

private:
    friend struct ELinkTelemetryTestProbe;

    struct TelemetryMessage {
        unsigned char type{};
        QByteArray data;
        unsigned short id{};
    };

    void addMessageInQueue(unsigned char type, const QByteArray& data);
    void addPlaneId2(QByteArray& data) const;
    void sendPosAtt2(float roll, float pitch, float yaw, float airSpeed);
    void sendGPSPos2(qlonglong time, int lat, int lon, int alt);
    void sendCurrentMode2(qlonglong time, unsigned char mode);
    void sendPlan2(qlonglong time, const Plan& plan);
    void sendModemCommand(const QByteArray& data);
    void emitTargetFromRetainedELinkMessage(bool includeKinematics);
    void replaceSocket(QUdpSocket*& socket);
    void renameWifi(const QByteArray& datagram);
    QString currentWifiName() const;

    unsigned char version_{};
    unsigned char configuration_{};
    unsigned short planeId_{};
    unsigned short planeSid_{};
    QString planeUid_;
    unsigned char telemetryStatus_{};
    unsigned short telemetryMessageId_{};
    QList<TelemetryMessage> messages_;
    QUdpSocket* telemetrySocket_{};
    QUdpSocket* telemetrySocket2_{};
    QUdpSocket* commandReservationSocket_{};
    QString hostAddress_;
    unsigned short telemetryPort_{};
    unsigned short telemetryPort2_{};
    QUdpSocket* modemCommandSocket_{};
    QProcess* pingProcess_{};
    bool connected_{};
    unsigned char videoStreamStatus_{};
    bool directUdpMode_{true};
    quint64 lastLoraMessageId_{};
    qlonglong lastLoraPingTime_{};
    std::int32_t lastTargetLat_{};
    std::int32_t lastTargetLon_{};
    qlonglong launcherEpoch_{};
    qlonglong launcherIntervalMs_{200};
    QUdpSocket* epLaunchSocket_{};
    unsigned short epLaunchReceivePort_{13002};
    unsigned short epLaunchSendPort_{13001};
    QString epLauncherAddress_;
    QString epConfiguratorAddress_;
    qlonglong configuratorEpoch_{};
    qlonglong configuratorIntervalMs_{1000};
    unsigned char currentMode_{};
    bool launched_{};
    bool planPresent_{};
    bool launchReady_{};
};
