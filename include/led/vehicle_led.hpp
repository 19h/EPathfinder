#pragma once

#include <QObject>
#include <QTimer>

#include <cstdint>

class VehicleLed : public QObject {
    Q_OBJECT

public:
    // The ELF preserves both enum types and the integer values used by control
    // flow, but it does not retain the original enumerator identifiers.
    enum class Type : int {
        Type0 = 0,
        Type1 = 1,
    };

    enum class State : int {
        Off = 0,
        On = 1,
        BlinkInitiallyOn = 2,
        BlinkInitiallyOff = 3,
    };

    explicit VehicleLed(QObject* parent = nullptr);

    void setPin(int pin) noexcept;
    void init(State state) noexcept;
    void setLedEnabled(bool enabled) noexcept;
    void on();
    void off();
    void blink(bool initiallyOff);
    void setState(State state);

private slots:
    void changeBlink();

private:
    std::uint8_t pin_ = 0;
    State state_ = State::Off;
    bool blinkValue_ = false;
    QTimer blinkTimer_;
};

class GPIOController : public QObject {
    Q_OBJECT

public:
    GPIOController(int led0Pin,
                   int led1Pin,
                   int unusedPin2,
                   int unusedPin3,
                   int unusedPin4,
                   QObject* parent = nullptr);

    void Init();

public slots:
    void setLedState(VehicleLed::Type type, VehicleLed::State state);
    void armTestOn();
    void armTestOff();
    void armOn();
    void selfDestruction();

private:
    VehicleLed leds_[2];
};
