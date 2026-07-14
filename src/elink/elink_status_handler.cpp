#include "elink/elink_status_handler.hpp"

#include <QByteArray>
#include <QTimer>

QString led2string(const LedState state)
{
    switch (state) {
    case LedState::None:
        return QStringLiteral("None");
    case LedState::Ready:
        return QStringLiteral("Ready");
    case LedState::NotReady:
        return QStringLiteral("NotReady");
    case LedState::Armed:
        return QStringLiteral("Armed");
    default:
        return QStringLiteral("UNKNOWN");
    }
}

ELinkStatusHandler::ELinkStatusHandler(
    ELinkCommunicator* const communicator)
    : ELinkAbstractHandler(communicator)
{
    QTimer::singleShot(100, this, &ELinkStatusHandler::getBoardVersion);
}

void ELinkStatusHandler::getBoardVersion()
{
    if (boardVersion_ != 0U) {
        return;
    }
    sendProtocolMessage(0x02U, QByteArray(1, '\0'));
    QTimer::singleShot(1000, this, &ELinkStatusHandler::getBoardVersion);
}

void ELinkStatusHandler::processMessage()
{
    switch (messageType_) {
    case 0x80U: {
        std::uint32_t voltage = 0U;
        readUInt32(2, voltage);
        if (batteryVoltage_ != voltage) {
            batteryVoltage_ = voltage;
            emit batteryVoltageChanged(voltage);
        }
        break;
    }
    case 0xb1U: {
        std::uint8_t state = 0U;
        readUInt8(3, state);
        if (buttonState_ != state) {
            buttonState_ = state;
            emit buttonStateChanged(state != 0U);
        }
        break;
    }
    case 0x02U: {
        std::uint32_t revision = 0U;
        std::uint32_t commit = 0U;
        std::uint32_t flags = 0U;
        std::uint16_t board = 0U;
        std::uint16_t option = 0U;
        readUInt32(2, revision);
        readUInt32(6, commit);
        readUInt32(10, flags);
        readUInt16(14, board);
        readUInt16(16, option);
        Q_UNUSED(revision)
        Q_UNUSED(commit)
        Q_UNUSED(flags)
        Q_UNUSED(option)
        if (board != 0U) {
            boardVersion_ = board;
        }
        break;
    }
    case 0x04U: {
        std::uint8_t value = 0U;
        readUInt8(2, value);
        sendProtocolMessage(0x04U,
                            QByteArray(1, static_cast<char>(value)));
        break;
    }
    default:
        break;
    }
}

void ELinkStatusHandler::setLedState(const LedState state)
{
    std::uint8_t led = notReadyLed_;
    std::uint8_t mode = 5U;
    switch (state) {
    case LedState::None:
        led = 0U;
        mode = 0U;
        break;
    case LedState::Ready:
        led = readyLed_;
        mode = 0U;
        break;
    case LedState::NotReady:
        led = notReadyLed_;
        mode = 0U;
        break;
    case LedState::Armed:
        led = readyLed_;
        mode = static_cast<std::uint8_t>(LedState::Armed);
        break;
    default:
        break;
    }

    QByteArray body;
    body.append(static_cast<char>(led));
    body.append(static_cast<char>(mode));
    sendProtocolMessage(0xb0U, body);
}
