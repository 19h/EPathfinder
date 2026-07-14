#pragma once

#include <cstdint>

// Original symbol: _Z5crc16Pht, RVA 0x25c38, size 0x58 bytes.
// CRC-16/CCITT-FALSE: poly=0x1021, init=0xffff, refin=false,
// refout=false, xorout=0x0000.
std::uint16_t crc16(std::uint8_t* data, std::uint16_t size) noexcept;

inline std::uint16_t crc16(const std::uint8_t* data,
                           std::uint16_t size) noexcept
{
    return crc16(const_cast<std::uint8_t*>(data), size);
}

