#include "elink/elink_shumodav_handler.hpp"

#include <limits>

ELinkShumodavHandler::ELinkShumodavHandler(
    ELinkCommunicator* const communicator)
    : ELinkAbstractHandler(communicator)
{
}

void ELinkShumodavHandler::processMessage()
{
    if (messageType_ != 0xc0U) {
        return;
    }

    std::int16_t count = 0;
    readInt16(4, count);
    int strongestCombinedPower = -1000;
    std::int16_t selectedPowerAzimuth = 0;
    std::int16_t selectedPowerElevation = 0;
    std::int16_t selectedAngleAzimuth = 0;
    std::int16_t selectedAngleElevation = 0;

    constexpr int firstItemOffset = 8;
    constexpr int itemSize = 26;
    for (int i = 0; i < count; ++i) {
        const int item = firstItemOffset + i * itemSize;
        std::int16_t statusAzimuth = 0;
        std::int16_t statusElevation = 0;
        std::int16_t powerAzimuth = 0;
        std::int16_t powerElevation = 0;
        std::int16_t angleAzimuth = 0;
        std::int16_t angleElevation = 0;
        readInt16(item + 2, statusAzimuth);
        readInt16(item + 4, statusElevation);
        readInt16(item + 10, powerAzimuth);
        readInt16(item + 12, powerElevation);
        readInt16(item + 18, angleAzimuth);
        readInt16(item + 20, angleElevation);

        if (statusAzimuth == 1 && statusElevation == 1) {
            const int combinedPower = static_cast<int>(powerAzimuth)
                + static_cast<int>(powerElevation);
            if (combinedPower > strongestCombinedPower) {
                strongestCombinedPower = combinedPower;
                selectedPowerAzimuth = powerAzimuth;
                selectedPowerElevation = powerElevation;
                selectedAngleAzimuth = angleAzimuth;
                selectedAngleElevation = angleElevation;
            }
        }
    }

    if (strongestCombinedPower != -1000) {
        emit target(selectedPowerAzimuth,
                    selectedPowerElevation,
                    selectedAngleAzimuth,
                    selectedAngleElevation);
    }
}
