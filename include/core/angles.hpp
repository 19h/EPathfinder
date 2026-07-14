#pragma once

#include <QVector>

// Arithmetic circular mean used by interceptor.cpp. Inputs and output are
// degrees in [0, 360] under the original call contract.
float averageAngle(const QVector<float>& angles) noexcept;

