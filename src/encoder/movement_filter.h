#pragma once
#include "movement_budget.h"
#include "position_math.h"
namespace position_math {
class MovementFilter {
public:
    MovementFilter(uint16_t initial, uint32_t rate, uint16_t capacity)
        : angle_(initial), lastDelta_(0), budget_(rate, capacity) {}
    uint16_t angle() const { return angle_; }
    uint16_t lastDelta() const { return lastDelta_; }
    bool update(uint16_t sample, uint32_t elapsedMs) {
        lastDelta_ = 0;
        budget_.accrue(elapsedMs);
        int32_t delta;
        if (!angularDelta(angle_, sample, delta) || delta < 0) return false;
        if (!budget_.consume(static_cast<uint16_t>(delta))) return false;
        angle_ = sample;
        lastDelta_ = static_cast<uint16_t>(delta);
        return true;
    }
private:
    uint16_t angle_;
    uint16_t lastDelta_;
    MovementBudget budget_;
};
}
