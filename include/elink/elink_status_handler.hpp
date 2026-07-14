#pragma once

#include "elink/elink_abstract_handler.hpp"

#include <QString>

enum class LedState {
    None = 0,
    Ready = 1,
    NotReady = 2,
    Armed = 3,
};

QString led2string(LedState state);

class ELinkStatusHandler final : public ELinkAbstractHandler {
    Q_OBJECT

public:
    explicit ELinkStatusHandler(ELinkCommunicator* communicator);

signals:
    void batteryVoltageChanged(unsigned int voltage);
    void buttonStateChanged(bool pressed);

public slots:
    void processMessage() override;
    void setLedState(LedState state);
    void getBoardVersion();

private:
    std::uint32_t batteryVoltage_{};
    std::uint8_t buttonState_{0xffU};
    std::uint8_t readyLed_{1U};
    std::uint8_t notReadyLed_{2U};
    std::uint16_t boardVersion_{};
};
