#pragma once

#include "mavlink/mavlink_communicator.hpp"

#include <QObject>
#include <QString>

class AbstractHandler : public QObject {
    Q_OBJECT

public:
    explicit AbstractHandler(MavLinkCommunicator* communicator);

    void setName(const QString& name);

public slots:
    virtual void initIntervals() = 0;

protected:
    void setInterval(int messageId, float intervalMicroseconds);

    MavLinkCommunicator* communicator_;
    QString name_;

protected slots:
    virtual void processMessage(const mavlink_message_t& message) = 0;
    virtual void backupProcessMessage(const QString& sender,
                                      const mavlink_message_t& message,
                                      unsigned char linkId);
};
