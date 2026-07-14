#pragma once

#include "mavlink/abstract_handler.hpp"

class AttitudeHandler : public AbstractHandler {
    Q_OBJECT

public:
    explicit AttitudeHandler(MavLinkCommunicator* communicator,
                             int attitudeIntervalMicroseconds);

signals:
    void attitudeEvent(uint32_t time_boot_ms,
                       float pitch,
                       float yaw,
                       float roll,
                       float pitchSpeed,
                       float yawSpeed,
                       float rollSpeed);
    void backupAttitudeEvent(unsigned char linkId,
                             uint32_t time_boot_ms,
                             float pitch,
                             float yaw,
                             float roll,
                             float pitchSpeed,
                             float yawSpeed,
                             float rollSpeed);
    void ahrsError(float rp, float yaw);

public slots:
    void initIntervals() override;
    void processMessage(const mavlink_message_t& message) override;
    void backupProcessMessage(const QString& sender,
                              const mavlink_message_t& message,
                              unsigned char linkId) override;
    void setAHRS(qlonglong time, float roll, float pitch, float yaw);

private:
    float attitudeIntervalMicroseconds_{};
    uint32_t lastAttitudeTime_{};
};
