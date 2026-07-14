#include "elink/elink_abstract_handler.hpp"

#include "elink/elink_communicator.hpp"
#include "core/crc.hpp"

#include <QDateTime>

#include <cstring>

ELinkAbstractHandler::ELinkAbstractHandler(
    ELinkCommunicator* const communicator)
    : QObject(communicator)
    , communicator_(communicator)
{
    connect(communicator_,
            &ELinkCommunicator::messageReceived,
            this,
            qOverload<const QByteArray&>(&ELinkAbstractHandler::processMessage));
}

qlonglong ELinkAbstractHandler::messageTime() const
{
    return messageTime_;
}

std::uint8_t ELinkAbstractHandler::messageType() const
{
    return messageType_;
}

std::uint8_t ELinkAbstractHandler::messageSize() const
{
    return messageSize_;
}

void ELinkAbstractHandler::processMessage(const QByteArray& message)
{
    messageTime_ = QDateTime::currentMSecsSinceEpoch()
        - communicator_->messageDelay();
    messageData_ = message.constData();
    messageType_ = message.size() > 1
        ? static_cast<std::uint8_t>(message[1])
        : 0U;
    messageSize_ = static_cast<std::uint8_t>(message.size());
    processMessage();
}

void ELinkAbstractHandler::readUInt8(const int offset,
                                     std::uint8_t& value) const
{
    value = static_cast<std::uint8_t>(messageData_[offset]);
}

void ELinkAbstractHandler::readInt16(const int offset,
                                     std::int16_t& value) const
{
    const auto* data = reinterpret_cast<const std::uint8_t*>(messageData_)
        + offset;
    value = static_cast<std::int16_t>(
        data[0] | (static_cast<std::uint16_t>(data[1]) << 8U));
}

void ELinkAbstractHandler::readUInt16(const int offset,
                                      std::uint16_t& value) const
{
    const auto* data = reinterpret_cast<const std::uint8_t*>(messageData_)
        + offset;
    value = static_cast<std::uint16_t>(
        data[0] | (static_cast<std::uint16_t>(data[1]) << 8U));
}

void ELinkAbstractHandler::readUInt32(const int offset,
                                      std::uint32_t& value) const
{
    const auto* data = reinterpret_cast<const std::uint8_t*>(messageData_)
        + offset;
    value = static_cast<std::uint32_t>(data[0])
        | (static_cast<std::uint32_t>(data[1]) << 8U)
        | (static_cast<std::uint32_t>(data[2]) << 16U)
        | (static_cast<std::uint32_t>(data[3]) << 24U);
}

void ELinkAbstractHandler::readInt(const int offset, int& value) const
{
    std::uint32_t raw = 0U;
    readUInt32(offset, raw);
    value = static_cast<std::int32_t>(raw);
}

void ELinkAbstractHandler::readFloat(const int offset, float& value) const
{
    std::uint32_t raw = 0U;
    readUInt32(offset, raw);
    static_assert(sizeof(raw) == sizeof(value));
    std::memcpy(&value, &raw, sizeof(value));
}

void ELinkAbstractHandler::sendProtocolMessage(
    const std::uint8_t id,
    const QByteArray& body)
{
    QByteArray payload;
    payload.reserve(2 + body.size() + 2);
    payload.append(static_cast<char>(sequence_++));
    payload.append(static_cast<char>(id));
    payload.append(body);

    const auto checksum = crc16(
        reinterpret_cast<const std::uint8_t*>(payload.constData()),
        static_cast<std::uint16_t>(payload.size()));
    payload.append(static_cast<char>(checksum & 0xffU));
    payload.append(static_cast<char>((checksum >> 8U) & 0xffU));

    QByteArray frame;
    frame.reserve(4 + payload.size());
    frame.append(static_cast<char>(0xd0));
    frame.append(static_cast<char>(0x0d));
    const auto length = static_cast<std::uint16_t>(payload.size());
    frame.append(static_cast<char>(length & 0xffU));
    frame.append(static_cast<char>((length >> 8U) & 0xffU));
    frame.append(payload);
    communicator_->sendMessageOnAllLinks(frame);
}
