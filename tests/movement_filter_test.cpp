#include "../src/encoder/movement_filter.h"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << "Failed line " << __LINE__ << ": " << #x << '\n'; std::exit(EXIT_FAILURE); } } while (false)
int main() {
    position_math::MovementFilter filter(65024, 100, 512);
    CHECK(filter.update(0, 5120) && filter.angle() == 0);
    CHECK(!filter.update(1024, 5120) && filter.angle() == 0);
    CHECK(filter.update(512, 0) && filter.angle() == 512);
    CHECK(!filter.update(0, 5120) && filter.angle() == 512);
    CHECK(filter.update(512, 0) && filter.angle() == 512);
    CHECK(filter.update(1024, 0) && filter.angle() == 1024);
    CHECK(!filter.update(33792, 5120) && filter.angle() == 1024);
    std::cout << "Movement filter OK\n";
}
