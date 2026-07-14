#pragma once

#include "elink/elink_abstract_handler.hpp"

class ELinkInterceptorHandler final : public ELinkAbstractHandler {
    Q_OBJECT

public:
    explicit ELinkInterceptorHandler(ELinkCommunicator* communicator);

signals:
    void targetDetected();

public slots:
    void processMessage() override;

private:
    bool targetPresent_{};
};
