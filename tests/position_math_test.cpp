#include "../src/encoder/position_math.h"
#include <cassert>
#include <iostream>
int main() {
    uint16_t angle = 123;
    assert(position_math::normalizePosition(1, 128, angle) && angle == 512);
    for (uint32_t n : {128u, 256u, 4096u, 65536u}) {
        for (uint32_t raw = 0; raw < n; ++raw) {
            assert(position_math::normalizePosition(raw, n, angle));
            assert(angle == raw * (65536u / n));
        }
    }
    angle = 123;
    assert(!position_math::normalizePosition(-1, 128, angle) && angle == 123);
    assert(!position_math::normalizePosition(128, 128, angle) && angle == 123);
    assert(!position_math::normalizePosition(0, 65537, angle) && angle == 123);
    for (uint32_t n = 1; n <= 65536u; ++n) {
        for (uint32_t raw : {0u, n / 2, n - 1}) {
            assert(position_math::normalizePosition(raw, n, angle));
            const uint64_t scaled = uint64_t(raw) * 65536u;
            assert(uint64_t(angle) * n <= scaled);
            assert(uint64_t(uint32_t(angle) + 1) * n > scaled);
        }
        angle = 123;
        assert(!position_math::normalizePosition(n, n, angle) && angle == 123);
    }
    assert(!position_math::normalizePosition(0, 0, angle) && angle == 123);
    assert(!position_math::normalizePosition(INT32_MIN, 128, angle) && angle == 123);
    assert(!position_math::normalizePosition(INT32_MAX, 128, angle) && angle == 123);
    assert(!position_math::normalizePosition(0, UINT32_MAX, angle) && angle == 123);
    std::cout << "Normalization OK\n";
}
