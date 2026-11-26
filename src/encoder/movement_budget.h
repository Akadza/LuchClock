#pragma once
#include <stdint.h>
namespace position_math {
// Rate in normalized units per second; credit in thousandths of a unit.
// Starts empty unless the caller supplies a bounded initial credit. The caller supplies elapsed milliseconds and forward distance.
class MovementBudget {
public:
    MovementBudget(uint32_t rate, uint16_t capacity, uint16_t initialCredit = 0)
        : rate_(rate), capacity_(uint64_t(capacity) * 1000), credit_(uint64_t(initialCredit > capacity ? capacity : initialCredit) * 1000) {}
    void accrue(uint32_t elapsedMs) {
        const uint64_t added = uint64_t(rate_) * elapsedMs;
        const uint64_t room = capacity_ - credit_;
        credit_ += added > room ? room : added;
    }
    bool consume(uint16_t distance) {
        const uint64_t cost = uint64_t(distance) * 1000;
        if (cost > credit_) return false;
        credit_ -= cost;
        return true;
    }
private:
    uint32_t rate_;
    uint64_t capacity_;
    uint64_t credit_;
};
}
