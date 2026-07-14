#include "core/angles.hpp"

#include <algorithm>

float averageAngle(const QVector<float>& angles) noexcept
{
    if (angles.isEmpty()) {
        return 0.0F;
    }
    if (angles.size() == 1) {
        return angles.front();
    }

    const auto bounds = std::minmax_element(angles.cbegin(), angles.cend());
    if (*bounds.second - *bounds.first < 180.0F) {
        // This preserves the original implementation's integer accumulator and
        // integer division, verified from RVA 0x42bf4..0x42c18. Fractional
        // components are truncated after every addition.
        int sum = 0;
        for (const float angle : angles) {
            sum = static_cast<int>(static_cast<float>(sum) + angle);
        }
        return static_cast<float>(sum / angles.size());
    }

    float sum = 0.0F;
    for (const float angle : angles) {
        sum += angle > 180.0F ? angle - 360.0F : angle;
    }
    const float result = sum / static_cast<float>(angles.size());
    return result < 0.0F ? result + 360.0F : result;
}
