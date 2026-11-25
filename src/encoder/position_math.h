#pragma once
#include <stdint.h>
namespace position_math {
inline bool normalizePosition(int32_t raw, uint32_t resolution, uint16_t& angle) {
    if (resolution > UINT32_C(65536) || raw < 0 || static_cast<uint32_t>(raw) >= resolution) return false;
    angle = static_cast<uint16_t>(static_cast<uint32_t>(raw) * UINT32_C(65536) / resolution);
    return true;
}
inline bool angularDelta(uint16_t from, uint16_t to, int32_t& delta) {
    int32_t d = int32_t(to) - int32_t(from);
    if (d == 32768 || d == -32768) return false;
    if (d < -32768) d += 65536;
    else if (d > 32768) d -= 65536;
    delta = d;
    return true;
}
}
