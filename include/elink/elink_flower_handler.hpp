#pragma once

#include "elink/elink_abstract_handler.hpp"

class QTimerEvent;

class ELinkFlowerHandler final : public ELinkAbstractHandler {
    Q_OBJECT

public:
    explicit ELinkFlowerHandler(ELinkCommunicator* communicator);

    bool isConnected() const;

signals:
    void currentAHRS(float roll,
                     float pitch,
                     float yaw,
                     unsigned char gnss,
                     int lat,
                     int lon,
                     int alt,
                     float hdg,
                     float gs,
                     float as);
    void currentGPS(int lat, int lon);

public slots:
    void processMessage() override;
    void sendCurrentCoordinate(qlonglong time,
                               int lat,
                               int lon,
                               float conf);

protected:
    void timerEvent(QTimerEvent* event) override;

private:
    qlonglong lastGpsEventTime_{};
    qlonglong lastMessageTime_{};
    std::uint8_t gnssStatus_{};
};
