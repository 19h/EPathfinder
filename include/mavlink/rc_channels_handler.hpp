#pragma once

#include "mavlink/abstract_handler.hpp"

class RcChannelsHandler : public AbstractHandler {
    Q_OBJECT

public:
    explicit RcChannelsHandler(MavLinkCommunicator* communicator);

signals:
    void homingOn();
    void homingOff();
    void backupHomingOn();
    void backupHomingOff();
    void currentControls(uint16_t roll, uint16_t pitch, uint16_t throttle);
    void currentChannels(uint16_t chan05_raw,
                         uint16_t chan06_raw,
                         uint16_t chan07_raw,
                         uint16_t chan08_raw,
                         uint16_t chan09_raw,
                         uint16_t chan10_raw,
                         uint16_t chan11_raw,
                         uint16_t chan12_raw,
                         uint16_t chan13_raw,
                         uint16_t chan14_raw,
                         uint16_t chan15_raw,
                         uint16_t chan16_raw,
                         uint16_t chan17_raw,
                         uint16_t chan18_raw);

public slots:
    void initIntervals() override;
    void processMessage(const mavlink_message_t& message) override;
    void backupProcessMessage(const QString& sender,
                              const mavlink_message_t& message,
                              unsigned char linkId) override;

private:
    void updateHoming(uint16_t value, bool backup);

    bool homing_{};
    bool backupHoming_{};
    int homingOnThreshold_{1600};
    int homingOffThreshold_{1200};
};
