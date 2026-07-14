#include "core/crc.hpp"

#include <array>
#include <cstddef>

namespace {

constexpr std::array<std::uint16_t, 256> makeCrc16Table() noexcept
{
    std::array<std::uint16_t, 256> table{};
    for (std::size_t i = 0; i < table.size(); ++i) {
        std::uint16_t value = static_cast<std::uint16_t>(i << 8U);
        for (unsigned bit = 0; bit < 8; ++bit) {
            value = static_cast<std::uint16_t>(
                (value & 0x8000U) != 0U
                    ? static_cast<std::uint16_t>((value << 1U) ^ 0x1021U)
                    : static_cast<std::uint16_t>(value << 1U));
        }
        table[i] = value;
    }
    return table;
}

constexpr auto crc16table = makeCrc16Table();
static_assert(crc16table[0] == 0x0000U);
static_assert(crc16table[1] == 0x1021U);
static_assert(crc16table[255] == 0x1ef0U);

} // namespace

std::uint16_t crc16(std::uint8_t* data, const std::uint16_t size) noexcept
{
    std::uint16_t crc = 0xffffU;
    if (size == 0U) {
        return crc;
    }
    if (data == nullptr) {
        // The original has undefined behavior for nullptr with size > 0.
        // Returning the initial state makes the reconstructed boundary total.
        return crc;
    }

    for (std::uint16_t i = 0; i < size; ++i) {
        const auto index = static_cast<std::uint8_t>(data[i] ^ (crc >> 8U));
        crc = static_cast<std::uint16_t>(crc16table[index] ^ (crc << 8U));
    }
    return crc;
}

