#include "../src/encoder/position_math.h"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << "Failed line " << __LINE__ << ": " << #x << '\n'; std::exit(EXIT_FAILURE); } } while (false)
int main() {
    uint16_t angle = 123;
    struct Case { int32_t raw; uint32_t n; uint16_t expected; };
    const Case cases[] = {{0,128,0},{1,128,512},{32,128,16384},{64,128,32768},{96,128,49152},{127,128,65024},{1,4096,16},{1024,4096,16384},{2048,4096,32768},{3072,4096,49152},{4095,4096,65520},{65535,65536,65535},{1,1000,65},{999,1000,65470}};
    for (const auto& c : cases) {
        CHECK(position_math::normalizePosition(c.raw,c.n,angle));
        CHECK(angle == c.expected);
    }
    for (uint32_t n = 1, step = 65536; n <= 65536; n *= 2, step /= 2) {
        uint32_t expected = 0;
        for (uint32_t raw = 0; raw < n; ++raw, expected += step) {
            CHECK(position_math::normalizePosition(raw,n,angle));
            CHECK(angle == expected);
        }
        CHECK(expected == 65536);
    }
    for (uint32_t n = 1; n <= 65536; ++n) {
        for (uint32_t raw : {0u,n/2,n-1}) {
            CHECK(position_math::normalizePosition(raw,n,angle));
            const uint64_t scaled = uint64_t(raw)*65536u;
            CHECK(uint64_t(angle)*n <= scaled);
            CHECK(uint64_t(uint32_t(angle)+1)*n > scaled);
        }
        for (int32_t raw : {-1,static_cast<int32_t>(n),INT32_MIN,INT32_MAX}) {
            angle = 123;
            CHECK(!position_math::normalizePosition(raw,n,angle));
            CHECK(angle == 123);
        }
    }
    for (uint32_t n : {0u,65537u,UINT32_MAX}) {
        angle = 123;
        CHECK(!position_math::normalizePosition(0,n,angle));
        CHECK(angle == 123);
    }
    std::cout << "Normalization OK\n";
}
