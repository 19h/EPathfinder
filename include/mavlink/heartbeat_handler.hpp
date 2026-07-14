#pragma once

#include "mavlink/abstract_handler.hpp"

#include <QMap>
#include <QSet>
#include <QString>

#include <cstdint>

class QTimerEvent;

class HeartbeatHandler : public AbstractHandler {
    Q_OBJECT

public:
    explicit HeartbeatHandler(unsigned char id,
                              MavLinkCommunicator* communicator,
                              float airspeedRatio);

    void readAcceleration(bool enabled);
    double lastAcceleration() const;
    std::uint16_t lastFlowXValue() const;
    std::uint16_t lastFlowYValue() const;
    void setRocketLaunch(bool enabled);
    void setAccDelay(unsigned int milliseconds);
    void rawIMULastValues(qlonglong& time,
                          short& xacc,
                          short& yacc,
                          short& zacc,
                          short& xgyro,
                          short& ygyro,
                          short& zgyro,
                          short& xmag,
                          short& ymag,
                          short& zmag) const;

signals:
    void currentModeChanged(int mode);
    void currentMissionItemChanged(int idx);
    void homingOn();
    void homingOff();
    void launchAcceleration();
    void armed();
    void disarmed();
    void compassStatusChanged(bool enabled);
    void mavlinkParamValue(const QString& name, float value);
    void airspeedEvent(float airSpeed, float groundSpeed);
    void distanceSensorEvent(unsigned int minDist,
                             unsigned int maxDist,
                             unsigned int curDist);
    void gimbalDeviceAttitudeStatus(qlonglong time,
                                    unsigned char compId,
                                    unsigned char gimbalDeviceId,
                                    float qw,
                                    float qx,
                                    float qy,
                                    float qz,
                                    float angular_velocity_x,
                                    float angular_velocity_y,
                                    float angular_velocity_z,
                                    float deltaYaw,
                                    float deltaYawVelocity);
    void isValidArdupilotVersion();
    void backupBattery(unsigned char id, short value);

public slots:
    void initIntervals() override;
    void processMessage(const mavlink_message_t& message) override;
    void backupProcessMessage(const QString& sender,
                              const mavlink_message_t& message,
                              unsigned char linkId) override;
    void setManualAccelerationEvent();
    void showMessageOnOSD(const QString& message);
    void readParam(const QString& name);
    void writeParam(const QString& name, float value);
    void enableAccelLog();
    void disableAccelLog();
    void disableOtherMessages();

protected:
    void timerEvent(QTimerEvent* event) override;

private slots:
    void sendOsdMessage();
    void writeParams();
    void readParams();
    void readArdupilotVersion();

private:
    std::uint8_t lastSequence_{};
    std::uint8_t id_{};
    int currentMode_{1000};
    int backupCurrentMode_{1000};
    bool armed_{};
    bool compassEnabled_{};
    int currentMissionItem_{-1};
    qint64 lastAccelerationSquared_{};
    std::uint16_t lastFlowX_{};
    std::uint16_t lastFlowY_{};
    qint64 accelerationThresholdSquared_{25000000};
    bool accelerationEventSent_{};
    bool readAcceleration_{};
    bool armedInitialized_{};
    float airspeedRatio_{};
    QString osdMessage_;
    qint64 reserved_{};
    unsigned int accelerationDelayMs_{80};
    bool rocketLaunch_{};
    bool accelerationLog_{};
    QSet<QString> pendingParameters_;
    QMap<QString, float> writtenParameters_;
    int parameterReadDelayMs_{300};
    qlonglong rawImuTime_{};
    short rawXacc_{};
    short rawYacc_{};
    short rawZacc_{};
    short rawXgyro_{};
    short rawYgyro_{};
    short rawZgyro_{};
    short rawXmag_{};
    short rawYmag_{};
    short rawZmag_{};
    QString ardupilotVersion_;
};
