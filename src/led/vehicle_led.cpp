#include "led/vehicle_led.hpp"

#include <QDebug>

#include <cstddef>

VehicleLed::VehicleLed(QObject* const parent)
    : QObject(parent)
{
    blinkTimer_.setInterval(500);
    connect(&blinkTimer_, &QTimer::timeout, this, &VehicleLed::changeBlink);
}

void VehicleLed::setPin(const int pin) noexcept
{
    pin_ = static_cast<std::uint8_t>(pin);
}

void VehicleLed::init(const State state) noexcept
{
    Q_UNUSED(state);
}

void VehicleLed::setLedEnabled(const bool enabled) noexcept
{
    Q_UNUSED(enabled);
}

void VehicleLed::changeBlink()
{
    blinkValue_ = !blinkValue_;
    setLedEnabled(blinkValue_);
}

void VehicleLed::on()
{
    blinkTimer_.stop();
    setLedEnabled(true);
}

void VehicleLed::off()
{
    blinkTimer_.stop();
    setLedEnabled(false);
}

void VehicleLed::blink(const bool initiallyOff)
{
    blinkValue_ = !initiallyOff;
    blinkTimer_.start();
    setLedEnabled(blinkValue_);
}

void VehicleLed::setState(const State state)
{
    if (state_ == state) {
        return;
    }

    state_ = state;
    if (state == State::Off) {
        off();
    } else if (state == State::On) {
        on();
    } else {
        blink(state == State::BlinkInitiallyOff);
    }
}

GPIOController::GPIOController(const int led0Pin,
                               const int led1Pin,
                               const int unusedPin2,
                               const int unusedPin3,
                               const int unusedPin4,
                               QObject* const parent)
    : QObject(parent)
    , leds_{VehicleLed{nullptr}, VehicleLed{nullptr}}
{
    Q_UNUSED(unusedPin2);
    Q_UNUSED(unusedPin3);
    Q_UNUSED(unusedPin4);
    leds_[0].setPin(led0Pin);
    leds_[1].setPin(led1Pin);
}

void GPIOController::Init()
{
    leds_[0].init(VehicleLed::State::Off);
    leds_[1].init(VehicleLed::State::Off);
}

void GPIOController::setLedState(const VehicleLed::Type type,
                                 const VehicleLed::State state)
{
    qDebug().noquote()
        << QStringLiteral("set %1 led state %2")
               .arg(static_cast<int>(type))
               .arg(static_cast<int>(state));
    leds_[static_cast<std::size_t>(type)].setState(state);
}

void GPIOController::armTestOn()
{
    qDebug() << "armTestOn";
}

void GPIOController::armTestOff()
{
    qDebug() << "armTestOff";
}

void GPIOController::armOn()
{
    qDebug() << "armOn";
}

void GPIOController::selfDestruction()
{
    qDebug() << "selfDestruction";
}
