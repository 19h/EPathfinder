#include "core/kalman.hpp"

#include <cmath>

Kalman::Kalman(QObject* parent)
    : QObject(parent)
{
}

float Kalman::kalman(const float measurement)
{
    float prediction = estimate_;
    float residual = measurement - prediction;

    if (std::abs(residual) / jumpScale_ > 0.25F) {
        // Constants are binary64 values loaded at RVAs 0x23fc60/0x23fc68.
        prediction = static_cast<float>(
            static_cast<double>(prediction) * 0.618
            + static_cast<double>(measurement) * 0.382);
        residual = measurement - prediction;
    }

    const double predictedCovariance = std::sqrt(
        static_cast<double>(q_ * q_ + covariance_ * covariance_));
    const double predictedVariance =
        predictedCovariance * predictedCovariance;
    const double gain = predictedVariance
        / (static_cast<double>(r_ * r_) + predictedVariance);

    const double estimate = static_cast<double>(prediction)
        + static_cast<double>(residual) * gain;
    const double covariance = std::sqrt(
        (1.0 - gain) * predictedVariance);

    covariance_ = static_cast<float>(covariance);
    estimate_ = static_cast<float>(estimate);
    return estimate_;
}

void Kalman::setQ(const float value) noexcept
{
    q_ = value;
}

void Kalman::setR(const float value) noexcept
{
    r_ = value;
}

