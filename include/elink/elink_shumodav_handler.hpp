#pragma once

#include "elink/elink_abstract_handler.hpp"

class ELinkShumodavHandler final : public ELinkAbstractHandler {
    Q_OBJECT

public:
    explicit ELinkShumodavHandler(ELinkCommunicator* communicator);

signals:
    void target(short powerAzim,
                short powerElev,
                short angleAzim,
                short angleElev);

public slots:
    void processMessage() override;
};
