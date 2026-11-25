#include "../src/encoder/movement_filter.h"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << "Failed line " << __LINE__ << ": " << #x << '\n'; std::exit(EXIT_FAILURE); } } while (false)
int main() {
    // Explicit one-ACE-step reverse tolerance. Never count a return to 57 as progress.
    position_math::MovementFilter jitter(56*512, 100, 512, 100, 512);
    CHECK(jitter.update(57*512, 5120) && jitter.lastDelta() == 512);
    for (int i = 0; i < 10; ++i) {
        CHECK(!jitter.update(56*512, 25) && jitter.valid() && jitter.angle() == 57*512 && jitter.lastDelta() == 0);
        CHECK(jitter.update(57*512, 25) && jitter.valid() && jitter.angle() == 57*512 && jitter.lastDelta() == 0);
    }
    position_math::MovementFilter stuck(56*512, 100, 512, 100, 512);
    CHECK(stuck.update(57*512, 5120));
    for (int i = 0; i < 5; ++i) CHECK(!stuck.update(56*512, 25) && stuck.valid());
    CHECK(!stuck.update(56*512, 100) && !stuck.valid() && stuck.angle() == 57*512);
    // Reverse distance larger than the explicit tolerance is suspicious immediately.
    position_math::MovementFilter large(56*512, 100, 512, 100, 512);
    CHECK(large.update(57*512, 5120));
    CHECK(!large.update(55*512, 25) && large.angle() == 57*512);
    CHECK(!large.update(57*512, 100) && !large.valid());
    std::cout << "Reverse jitter OK\n";
}
