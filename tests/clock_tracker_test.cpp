#include "../src/encoder/clock_tracker.h"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << "Failed line " << __LINE__ << ": " << #x << '\n'; std::exit(EXIT_FAILURE); } } while (false)
int main() {
    position_math::ClockTracker clock(65024);
    CHECK(!clock.known());
    CHECK(clock.setHour(11) && clock.position() == 11u*65536u + 65024u);
    CHECK(clock.advance(0, 512) && clock.known() && clock.position() == 0);
    clock.loseContinuity();
    CHECK(!clock.known());
    CHECK(!clock.setHour(1));
    CHECK(!clock.advance(512, 512));
    clock.beginObservation(0);
    CHECK(clock.setHour(0) && clock.position() == 0);
    CHECK(!clock.setHour(12) && clock.position() == 0);
    CHECK(!clock.advance(1024, 512) && clock.position() == 0 && clock.angle() == 0);
    CHECK(!clock.advance(32768, 32768) && clock.position() == 0 && clock.angle() == 0);
    for (uint32_t step = 1; step <= 12u * 128u; ++step) {
        const uint16_t physical = static_cast<uint16_t>(step * 512u);
        const uint32_t expected = (step * 512u) % 786432u;
        CHECK(clock.advance(physical, 512) && clock.position() == expected && clock.angle() == physical && clock.known());
    }
    position_math::ClockTracker unanchored(65520);
    CHECK(unanchored.advance(0, 16) && !unanchored.known());
    unanchored.beginObservation(4096); // New physical baseline does not restore the old hour.
    CHECK(!unanchored.known() && unanchored.angle() == 4096);
    // Raw 125 + offset 2 = calibrated 127; next raw 126 crosses the dial hour.
    position_math::ClockTracker calibrated(position_math::applyOffset(64000, 1024));
    CHECK(calibrated.setHour(11) && calibrated.position() == 785920u);
    CHECK(calibrated.advance(position_math::applyOffset(64512, 1024), 512));
    CHECK(calibrated.position() == 0 && calibrated.known());
    // Recalibration is not movement and requires a new operator anchor.
    calibrated.beginObservation(position_math::applyOffset(64512, 2048));
    CHECK(!calibrated.known() && calibrated.angle() == 1024);
    CHECK(calibrated.advance(1536, 512) && !calibrated.known());
    CHECK(calibrated.setHour(3) && calibrated.position() == 198144u);
    std::cout << "Clock tracker OK\n";
}
