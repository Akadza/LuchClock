#pragma once
#include <stdint.h>
namespace position_math {
inline bool normalizePosition(int32_t raw, uint32_t resolution, uint16_t& angle) {
    if (resolution > UINT32_C(65536) || raw < 0 || static_cast<uint32_t>(raw) >= resolution) return false;
    angle = static_cast<uint16_t>(static_cast<uint32_t>(raw) * UINT32_C(65536) / resolution);
    return true;
}
}
