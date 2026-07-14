#pragma once

#include "mavlink/abstract_handler.hpp"

class TimeSyncHandler : public AbstractHandler {
    Q_OBJECT

public:
    explicit TimeSyncHandler(MavLinkCommunicator* communicator);

signals:
    void updateTimeOffset(qlonglong offset, int roundDt);

public slots:
    void initIntervals() override;
    void reset();
    void startSync();
    void processMessage(const mavlink_message_t& message) override;

private slots:
    void sendSyncMessage();
    void updateDeltaOffset();

private:
    void sendTimeOffset(qint64 offset, int roundDt);

    int sampleTarget_{40};
    qint64 offsetSum_{};
    int roundTripSum_{};
    int sampleCount_{};
    bool syncing_{};
    bool resetPending_{};
    qint64 lastSyncWallTime_{};
    qint64 lastOffset_{};
    int deltaOffset_{};
    int deltaTime_{};
    qint64 emittedOffset_{};
    int emittedRoundTrip_{};
    qint64 synchronizationCount_{};
};
