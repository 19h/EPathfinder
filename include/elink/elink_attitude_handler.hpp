#pragma once

#include "elink/elink_abstract_handler.hpp"

class ELinkAttitudeHandler final : public ELinkAbstractHandler {
    Q_OBJECT

public:
    explicit ELinkAttitudeHandler(ELinkCommunicator* communicator);

signals:
    void attitudeEvent(qlonglong time, float pitch, float roll, float yaw);
    void mgCfgStatus(unsigned char status);
    void mgInited();
    void mgReseted(unsigned char rc);

public slots:
    void processMessage() override;
    void mgReset();

private:
    std::uint8_t lastMgStatus_{};
};
