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
    std::cout << "Observation timeout OK\n";
}
