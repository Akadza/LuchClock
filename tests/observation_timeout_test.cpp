#include "../src/encoder/movement_filter.h"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << "Failed line " << __LINE__ << ": " << #x << '\n'; std::exit(EXIT_FAILURE); } } while (false)
int main() {
    using position_math::MovementFilter;
    MovementFilter mixed(512, 120, 1024, 500, 512);
    CHECK(mixed.update(512, 10000));
    for (int i=0; i<10; ++i) {
        CHECK(!mixed.update(1024, 25));
        CHECK(!mixed.update(0, 25));
    }
    CHECK(mixed.valid());
    CHECK(!mixed.update(1024, 25) && !mixed.valid());
    MovementFilter missing(512, 120, 1024, 500, 512);
    CHECK(missing.update(512, 10000));
    CHECK(!missing.update(1024, 25));
    CHECK(!missing.missing(250));
    CHECK(!missing.missing(250) && !missing.valid());
    MovementFilter first(65024, 120, 1024, 500, 512, 2, 512);
    CHECK(!first.update(0, 25));
    CHECK(first.update(0, 25) && first.lastDelta() == 512);
    CHECK(first.rebase(512));
    CHECK(first.lastDelta() == 0);
    CHECK(!first.update(1024, 25));
    CHECK(!first.update(1024, 25) && first.angle() == 512);
    std::cout << "Observation timeout OK\n";
}
