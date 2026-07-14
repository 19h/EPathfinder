#pragma once

#include "mavlink/abstract_handler.hpp"

class WindHandler : public AbstractHandler {
    Q_OBJECT

public:
    explicit WindHandler(MavLinkCommunicator* communicator);

signals:
    void windEvent(float direction, float speed, float speed_z);

public slots:
    void initIntervals() override;
    void processMessage(const mavlink_message_t& message) override;
    void backupProcessMessage(const QString& sender,
                              const mavlink_message_t& message,
                              unsigned char linkId) override;
};
