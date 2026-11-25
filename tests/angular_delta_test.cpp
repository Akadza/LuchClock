#include "../src/encoder/position_math.h"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << "Failed line " << __LINE__ << ": " << #x << '\n'; std::exit(EXIT_FAILURE); } } while (false)
int main() {
    int32_t delta = 123;
    CHECK(position_math::angularDelta(1000,1512,delta) && delta == 512);
    CHECK(position_math::angularDelta(65024,0,delta) && delta == 512);
    CHECK(position_math::angularDelta(0,65024,delta) && delta == -512);
    delta = 123;
    CHECK(!position_math::angularDelta(0,32768,delta) && delta == 123);
    // Independent oracle: walk the circle in both directions from every origin.
    for (uint32_t start = 0; start < 65536; ++start) {
        for (int32_t expected : {0,1,-1,16,-16,512,-512,32767,-32767}) {
            const uint16_t end = static_cast<uint16_t>(int32_t(start) + expected);
            CHECK(position_math::angularDelta(start,end,delta) && delta == expected);
        }
        delta = 123;
        CHECK(!position_math::angularDelta(start,static_cast<uint16_t>(start ^ 32768u),delta) && delta == 123);
    }
    // Every pair of AS5600 positions, including the ACE-128 grid.
    for (uint32_t start = 0; start < 65536; start += 16) {
        uint16_t end = static_cast<uint16_t>(start);
        for (int32_t step = 0; step < 2048; ++step) {
            CHECK(position_math::angularDelta(start,end,delta) && delta == step * 16);
            CHECK(position_math::angularDelta(end,start,delta) && delta == -step * 16);
            end = static_cast<uint16_t>(end + 16u);
        }
        delta = 123;
        CHECK(!position_math::angularDelta(start,end,delta) && delta == 123);
    }
    std::cout << "Angular delta OK\n";
}
