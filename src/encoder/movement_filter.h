#pragma once
#include "movement_budget.h"
#include "position_math.h"
namespace position_math {
class MovementFilter {
public:
    MovementFilter(uint16_t initial, uint32_t rate, uint16_t capacity)
        : angle_(initial), budget_(rate, capacity) {}
    uint16_t angle() const { return angle_; }
    bool update(uint16_t sample, uint32_t elapsedMs) {
        budget_.accrue(elapsedMs);
        int32_t delta;
        if (!angularDelta(angle_, sample, delta) || delta < 0) return false;
        if (!budget_.consume(static_cast<uint16_t>(delta))) return false;
        angle_ = sample;
        return true;
    }
private:
    uint16_t angle_;
    MovementBudget budget_;
};
}
