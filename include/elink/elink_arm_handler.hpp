#pragma once

#include "elink/elink_abstract_handler.hpp"

#include <QtGlobal>

#include <cstdint>

class ELinkArmHandler final : public ELinkAbstractHandler {
    Q_OBJECT

public:
    explicit ELinkArmHandler(ELinkCommunicator* communicator);

signals:
    void armTestOnEvent();
    void armTestOffEvent();

public slots:
    void processMessage() override;
    void armOn();
    void selfDestruction();

private slots:
    void sendArmMsg();

private:
    void sendMsg(std::uint8_t id, std::uint8_t value);

    std::uint8_t armTestState_{};
    qlonglong sendCounter_{};
    bool armed_{};
    bool selfDestruction_{};
};
