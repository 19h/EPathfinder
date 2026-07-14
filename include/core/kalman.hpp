#pragma once

#include <QObject>

// Recovered from kalman.cpp symbols and RVAs 0x464d8..0x46630.
class Kalman final : public QObject
{
    Q_OBJECT

public:
    explicit Kalman(QObject* parent = nullptr);

    float kalman(float measurement);
    void setQ(float value) noexcept;
    void setR(float value) noexcept;

    float q() const noexcept { return q_; }
    float r() const noexcept { return r_; }
    float covariance() const noexcept { return covariance_; }
    float estimate() const noexcept { return estimate_; }

private:
    float q_ = 0.02F;
    float r_ = 0.10F;
    float covariance_ = 1.0F;
    float estimate_ = 0.0F;
    float jumpScale_ = 50.0F;
};

