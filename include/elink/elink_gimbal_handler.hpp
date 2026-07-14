#pragma once

#include "elink/elink_abstract_handler.hpp"

class ELinkGimbalHandler final : public ELinkAbstractHandler {
    Q_OBJECT

public:
    explicit ELinkGimbalHandler(ELinkCommunicator* communicator);

public slots:
    void processMessage() override;
    void setServoPTZ(float roll,
                     float pitch,
                     float yaw,
                     float rollMaxSpeed,
                     float pitchMaxSpeed,
                     float yawMaxSpeed,
                     bool center = false,
                     bool bYawByNorth = false,
                     int gimbalId = 0);

private slots:
    void fixRoll();
    void unfixRoll();
    void setRollFixed(bool fixed);
    void setPitch(float angle, bool fixed);

private:
    std::uint8_t pitchAxis_{};
    std::uint8_t rollAxis_{1U};
};
