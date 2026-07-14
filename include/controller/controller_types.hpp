#pragma once

#include <QMetaType>

#include <cstdint>

// Copied as three qwords by ClientController::updateVehicleStatus.  Field
// positions are recovered from VehicleController's status JSON producer.
struct VehicleStatus {
    std::int32_t camera{};
    bool connected{};
    std::uint8_t reserved05[3]{};
    std::int32_t plan{};
    bool armed{};
    bool launched{};
    std::uint8_t reserved14[2]{};
    std::int32_t satellites{};
    bool vnav{};
    std::uint8_t reserved21[3]{};
};

static_assert(sizeof(VehicleStatus) == 24U);
Q_DECLARE_METATYPE(VehicleStatus)
