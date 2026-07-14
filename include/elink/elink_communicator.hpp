#pragma once

#include <QByteArray>
#include <QList>
#include <QObject>

#include <cstdint>

class AbstractLink;

class ELinkCommunicator : public QObject {
    Q_OBJECT

public:
    explicit ELinkCommunicator(QObject* parent = nullptr);

    std::uint16_t messageDelay() const;
    QList<AbstractLink*> links() const;

signals:
    void messageReceived(QByteArray message);

public slots:
    void addLink(AbstractLink* link);
    void removeLink(AbstractLink* link);
    void sendMessageOnAllLinks(const QByteArray& message);
    void sendMessage(const QByteArray& message, AbstractLink* link);
    void addMessageTimeDelay(int value);

protected slots:
    void onDataReceived(const QByteArray& data);

private:
    QList<AbstractLink*> links_;
    std::uint16_t messageDelayMs_{50};
    QByteArray currentMessage_;
    std::uint16_t framePosition_{};
    std::uint16_t messageLength_{};
    std::uint8_t messageByte0_{};
    std::uint8_t messageByte1_{};
};
