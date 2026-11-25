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
    position_math::MovementFilter magnetic(65520, 100, 16);
    for (int i=0; i<6; ++i) CHECK(magnetic.update(65520, 25));
    CHECK(!magnetic.update(0, 0) && magnetic.angle() == 65520);
    CHECK(magnetic.update(0, 10) && magnetic.angle() == 0);
    CHECK(!magnetic.update(32, 1000) && magnetic.angle() == 0);
    CHECK(magnetic.update(16, 0) && magnetic.angle() == 16);
    CHECK(!magnetic.update(0, 160) && magnetic.angle() == 16);
    CHECK(magnetic.update(16, 0) && magnetic.angle() == 16);
    CHECK(magnetic.update(32, 0) && magnetic.angle() == 32);
    CHECK(magnetic.lastDelta() == 16);
    CHECK(!magnetic.update(64, 0) && magnetic.lastDelta() == 0);
    CHECK(magnetic.update(32, 0) && magnetic.lastDelta() == 0);
    std::cout << "Movement filter OK\n";
}
