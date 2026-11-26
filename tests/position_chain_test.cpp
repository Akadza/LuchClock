#include "../src/encoder/position_math.h"
#include <cstdlib>
#include <iostream>

#define CHECK(x) do { if (!(x)) { std::cerr << "Failed line " << __LINE__ << ": " << #x << '\n'; std::exit(EXIT_FAILURE); } } while (false)

// Independent expected positions come from the raw step count, not from
// any production conversion or delta function.
static void fullTwelveHours(uint32_t resolution, uint16_t offset) {
    const uint32_t step = 65536u / resolution;
    uint16_t rawAngle = 0;
    CHECK(position_math::normalizePosition(0, resolution, rawAngle));
    uint16_t previousAngle = position_math::applyOffset(rawAngle, offset);
    uint32_t clock = 11u * 65536u + previousAngle;
    const uint32_t initialClock = clock;

    for (uint32_t count = 1; count <= resolution * 12u; ++count) {
        const uint32_t raw = count % resolution;
        CHECK(position_math::normalizePosition(static_cast<int32_t>(raw), resolution, rawAngle));
        const uint16_t measured = position_math::applyOffset(rawAngle, offset);
        const uint16_t expectedAngle = static_cast<uint16_t>((uint64_t(raw) * step + offset) % 65536u);
        CHECK(measured == expectedAngle);

        int32_t delta = 0;
        CHECK(position_math::angularDelta(previousAngle, measured, delta));
        CHECK(delta == static_cast<int32_t>(step));

        uint32_t advanced = 0;
        CHECK(position_math::advanceClockPosition(clock, delta, advanced));
        clock = advanced;
        const uint32_t expectedClock = static_cast<uint32_t>((uint64_t(initialClock) + uint64_t(count) * step) % 786432u);
        CHECK(clock == expectedClock);
        CHECK(clock % 65536u == measured);
        previousAngle = measured;
    }
    CHECK(clock == initialClock);
}

int main() {
    // Each driver has a different resolution; shared math must agree.
    fullTwelveHours(128, 0);
    fullTwelveHours(128, 65024);
    fullTwelveHours(4096, 0);
    fullTwelveHours(4096, 65520);

    // Position 124 with target 123 is one step ahead, not 127 steps behind.
    int32_t diff = 0;
    const uint32_t actual = 5u * 65536u + 124u * 512u;
    const uint32_t target = 5u * 65536u + 123u * 512u;
    CHECK(position_math::clockDelta(actual, target, diff) && diff == -512);
    CHECK(position_math::clockDelta(target, actual, diff) && diff == 512);

    // Same encoder angle does not imply the same hour.
    CHECK(position_math::clockDelta(65536u + 123u * 512u,
                                     2u * 65536u + 123u * 512u, diff) && diff == 65536);
    CHECK(position_math::clockDelta(11u * 65536u + 123u * 512u,
                                     123u * 512u, diff) && diff == 65536);

    // Changing calibration is not physical movement; the caller must retain
    // the last measured physical angle instead of feeding it as a new sample.
    uint16_t angle = 0;
    CHECK(position_math::normalizePosition(127, 128, angle) && angle == 65024);
    CHECK(position_math::applyOffset(angle, 0) == 65024);
    CHECK(position_math::applyOffset(angle, 512) == 0);

    std::cout << "Position chain OK\n";
}
