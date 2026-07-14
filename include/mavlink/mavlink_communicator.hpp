#pragma once

#include "link/abstract_link.hpp"

#include <ardupilotmega/mavlink.h>

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QMetaType>
#include <QObject>
#include <QString>

#include <cstdint>

Q_DECLARE_METATYPE(mavlink_message_t)

class MavLinkCommunicator : public QObject {
    Q_OBJECT

public:
    explicit MavLinkCommunicator(uint8_t systemId,
                                 uint8_t componentId,
                                 QObject* parent = nullptr);

    uint8_t systemId() const;
    uint8_t componentId() const;
    QList<AbstractLink*> links() const;

signals:
    void messageReceived(const mavlink_message_t& message);
    void backupMessageReceived(const QString& sender,
                               const mavlink_message_t& message,
                               unsigned char linkId);

public slots:
    void addLink(AbstractLink* link, uint8_t channel, int id);
    void removeLink(AbstractLink* link);
    void setSystemId(uint8_t systemId);
    void setComponentId(uint8_t componentId);
    void sendMessage(const mavlink_message_t& message, AbstractLink* link);
    void sendMessageOnLastReceivedLink(const mavlink_message_t& message);
    void sendMessageOnAllLinks(const mavlink_message_t& message);
    void sendMessageOnMainLink(const mavlink_message_t& message);
    void sendMessageOnBackupLink(const mavlink_message_t& message);

protected slots:
    void onDataReceived(const QByteArray& data);

private:
    struct LinkData {
        uint8_t channel{};
        uint8_t id{};
    };

    QMap<AbstractLink*, LinkData> links_;
    AbstractLink* lastReceivedLink_{};
    AbstractLink* mainLink_{};
    AbstractLink* backupLink_{};
    uint8_t systemId_{};
    uint8_t componentId_{};
    qint64 lastMessageTime_{};
};
