#pragma once

#include <QByteArray>
#include <QObject>

#include <cstdint>

class ELinkCommunicator;

class ELinkAbstractHandler : public QObject {
    Q_OBJECT

public:
    explicit ELinkAbstractHandler(ELinkCommunicator* communicator);

    qlonglong messageTime() const;
    std::uint8_t messageType() const;
    std::uint8_t messageSize() const;

protected slots:
    void processMessage(const QByteArray& message);
    virtual void processMessage() = 0;

protected:
    void readUInt8(int offset, std::uint8_t& value) const;
    void readInt16(int offset, std::int16_t& value) const;
    void readUInt16(int offset, std::uint16_t& value) const;
    void readUInt32(int offset, std::uint32_t& value) const;
    void readInt(int offset, int& value) const;
    void readFloat(int offset, float& value) const;
    void sendProtocolMessage(std::uint8_t id,
                             const QByteArray& body = {});

    ELinkCommunicator* communicator_{};
    qlonglong messageTime_{};
    std::uint8_t messageType_{};
    std::uint8_t messageSize_{};
    const char* messageData_{};
    std::uint8_t sequence_{};
};
