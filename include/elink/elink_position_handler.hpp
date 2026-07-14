#pragma once

#include "elink/elink_abstract_handler.hpp"

class ELinkPositionHandler final : public ELinkAbstractHandler {
    Q_OBJECT

public:
    explicit ELinkPositionHandler(ELinkCommunicator* communicator);

signals:
    void positionEvent(qlonglong time,
                       unsigned char status,
                       unsigned char satellites,
                       int lat,
                       int lon,
                       int alt,
                       float vx,
                       float vy,
                       float vz,
                       float groundSpeed,
                       float groundCourse);
    void backupPositionEvent(qlonglong time,
                             unsigned char status,
                             unsigned char satellites,
                             int lat,
                             int lon,
                             int alt,
                             float vx,
                             float vy,
                             float vz,
                             float groundSpeed,
                             float groundCourse);
    void barometricRawEvent(qlonglong time, int pressure, int temp);
    void magneticEvent(qlonglong time, int xmag, int ymag, int zmag);
    void barometricAltEvent(qlonglong time, int alt);
    void distanceSensorEvent(unsigned int minDist,
                             unsigned int maxDist,
                             unsigned int curDist);
    void launchAcceleration();

public slots:
    void processMessage() override;
    void setManualAccelerationEvent();
    void sendAirspeed(float value);

private:
    void sendLaunchEvent();

    bool accelerationDetected_{};
};
