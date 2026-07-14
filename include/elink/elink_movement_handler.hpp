#pragma once

#include "elink/elink_abstract_handler.hpp"

// The misspelling is part of the recovered Qt meta-object ABI.
class ELinkMovemenetHandler final : public ELinkAbstractHandler {
    Q_OBJECT

public:
    explicit ELinkMovemenetHandler(ELinkCommunicator* communicator);

signals:
    void accelerationEvent(qlonglong time,
                           unsigned int xacc,
                           unsigned int yacc,
                           unsigned int zacc,
                           int temp);
    void airspeedRawEvent(qlonglong time, int pressure, int temp);
    void airspeedEvent(qlonglong time, float speed);

public slots:
    void processMessage() override;
};
