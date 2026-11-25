#include "../src/encoder/movement_filter.h"
#include "../src/encoder/clock_tracker.h"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << "Failed line " << __LINE__ << ": " << #x << '\n'; std::exit(EXIT_FAILURE); } } while (false)
int main() {
    position_math::MovementFilter filter(56*512, 100, 1024, 100);
    position_math::ClockTracker clock(56*512);
    CHECK(clock.setHour(5));
    CHECK(filter.healthy());
    CHECK(!filter.update(107*512, 25) && !filter.healthy());
    CHECK(!filter.update(56*512, 25));
    CHECK(filter.update(56*512, 25) && filter.healthy());
    filter.invalidate(); // I2C device vanished; caller must stop motor immediately.
    clock.loseContinuity();
    CHECK(!filter.valid() && !filter.healthy() && !clock.known());
    CHECK(!filter.update(57*512, 10000) && filter.angle() == 56*512 && filter.lastDelta() == 0);
    CHECK(!clock.setHour(5)); // Only a fresh observation and manual re-anchoring can restore hour.
    std::cout << "Sensor loss OK\n";
}
