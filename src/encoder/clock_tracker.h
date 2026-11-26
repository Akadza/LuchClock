#pragma once
#include <stdint.h>
#include "position_math.h"
namespace position_math {
// Tracks the 12-hour position only while continuity and the operator's hour are known.
// All movement deltas must already have been confirmed by the physical filter.
class ClockTracker {
public:
    explicit ClockTracker(uint16_t initialAngle)
        : angle_(initialAngle), position_(0), known_(false), continuous_(true) {}
    bool known() const { return known_; }
    uint16_t angle() const { return angle_; }
    uint32_t position() const { return position_; }
    bool setHour(uint8_t hour) {
        if (!continuous_ || hour >= 12) return false;
        position_ = uint32_t(hour) * UINT32_C(65536) + angle_;
        known_ = true;
        return true;
    }
    bool advance(uint16_t newAngle, uint16_t confirmedDelta) {
        if (!continuous_ || confirmedDelta >= UINT16_C(32768) ||
            static_cast<uint16_t>(uint32_t(angle_) + confirmedDelta) != newAngle) return false;
        uint32_t next = 0;
        if (known_ && !advanceClockPosition(position_, confirmedDelta, next)) return false;
        angle_ = newAngle;
        if (known_) position_ = next;
        return true;
    }
    void loseContinuity() { known_ = false; continuous_ = false; }
    // Re-acquire a fresh physical baseline; never restore the hour implicitly.
    void beginObservation(uint16_t newAngle) {
        angle_ = newAngle;
        known_ = false;
        continuous_ = true;
    }
private:
    uint16_t angle_;
    uint32_t position_;
    bool known_;
    bool continuous_;
};
}
