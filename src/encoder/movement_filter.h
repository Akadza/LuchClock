#pragma once
#include "movement_budget.h"
#include "position_math.h"
namespace position_math {
class MovementFilter {
public:
    MovementFilter(uint16_t initial, uint32_t rate, uint16_t capacity, uint32_t timeoutMs = UINT32_MAX)
        : angle_(initial), lastDelta_(0), budget_(rate, capacity), timeoutMs_(timeoutMs), badMs_(0), suspicious_(false), valid_(true), recoveryCount_(0) {}
    bool valid() const { return valid_; }
    uint16_t angle() const { return angle_; }
    uint16_t lastDelta() const { return lastDelta_; }
    bool update(uint16_t sample, uint32_t elapsedMs) {
        lastDelta_ = 0;
        if (!valid_) return false;
        if (suspicious_) {
            const uint32_t room = timeoutMs_ - badMs_;
            badMs_ += elapsedMs > room ? room : elapsedMs;
            if (badMs_ >= timeoutMs_) { valid_ = false; return false; }
        }
        budget_.accrue(elapsedMs);
        int32_t delta;
        if (!angularDelta(angle_, sample, delta) || delta < 0) return reject();
        if (!budget_.consume(static_cast<uint16_t>(delta))) return reject();
        if (suspicious_ && ++recoveryCount_ >= 2) {
            suspicious_ = false;
            badMs_ = 0;
            recoveryCount_ = 0;
        }
        angle_ = sample;
        lastDelta_ = static_cast<uint16_t>(delta);
        return true;
    }
private:
    bool reject() {
        recoveryCount_ = 0;
        suspicious_ = true;
        if (badMs_ >= timeoutMs_) valid_ = false;
        return false;
    }
    uint16_t angle_;
    uint16_t lastDelta_;
    MovementBudget budget_;
    uint32_t timeoutMs_;
    uint32_t badMs_;
    bool suspicious_;
    bool valid_;
    uint8_t recoveryCount_;
};
}
