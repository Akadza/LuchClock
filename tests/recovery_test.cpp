#include "../src/encoder/movement_filter.h"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << "Failed line " << __LINE__ << ": " << #x << '\n'; std::exit(EXIT_FAILURE); } } while (false)
int main() {
    position_math::MovementFilter f(56*512, 100, 1024, 100);
    CHECK(!f.update(107*512, 5625));
    CHECK(!f.update(57*512, 25) && f.angle() == 56*512 && f.lastDelta() == 0);
    CHECK(f.update(57*512, 25) && f.angle() == 57*512 && f.lastDelta() == 512);
    std::cout << "Recovery OK\n";
}
