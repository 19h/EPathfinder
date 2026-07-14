#pragma once

#include "mavlink/abstract_handler.hpp"

class GpsHandler : public AbstractHandler {
    Q_OBJECT

public:
    explicit GpsHandler(MavLinkCommunicator* communicator);

    uint8_t satelliteCount() const;

signals:
    void satelliteCountChanged();
    void positionEvent(uint32_t time_boot_ms,
                       int32_t lat,
                       int32_t lon,
                       int32_t alt,
                       int32_t mslAlt,
                       int16_t vx,
                       int16_t vy,
                       int16_t vz,
                       uint16_t hdg);
    void rawPositionEvent(uint32_t time_boot_ms,
                          int32_t lat,
                          int32_t lon,
                          int32_t mslAlt);

public slots:
    void initIntervals() override;
    void processMessage(const mavlink_message_t& message) override;
    void backupProcessMessage(const QString& sender,
                              const mavlink_message_t& message,
                              unsigned char linkId) override;

private:
    bool satelliteCountInitialized_{};
    uint8_t satelliteCount_{};
};
