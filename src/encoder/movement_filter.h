#pragma once
#include "movement_budget.h"
#include "position_math.h"
namespace position_math {
class MovementFilter {
public:
    MovementFilter(uint16_t initial, uint32_t rate, uint16_t capacity, uint32_t timeoutMs = UINT32_MAX, uint16_t reverseTolerance = 0, uint8_t stableSamples = 2, uint16_t initialCredit = 0)
        : angle_(initial), lastDelta_(0), budget_(rate, capacity, initialCredit), timeoutMs_(timeoutMs),
          unconfirmedMs_(0), suspicious_(false), valid_(true), reverseActive_(false),
          pending_(false), candidate_(initial), stableSamples_(stableSamples < 2 ? 2 : stableSamples),
          count_(0), reverseTolerance_(reverseTolerance) {}
    bool valid() const { return valid_; }
    bool healthy() const { return valid_ && !suspicious_ && !pending_ && !reverseActive_; }
    void invalidate() {
        valid_ = false;
        suspicious_ = true;
        lastDelta_ = 0;
        pending_ = false;
        count_ = 0;
    }
    uint16_t angle() const { return angle_; }
    uint16_t lastDelta() const { return lastDelta_; }
    // Only called on a healthy, stopped observation after successful calibration.
    // Preserve physical movement credit; an offset change is not movement.
    bool rebase(uint16_t angle) {
        if (!healthy()) return false;
        angle_ = angle;
        lastDelta_ = 0;
        return true;
    }
    bool missing(uint32_t elapsedMs) {
        if (!beginSample(elapsedMs)) return false;
        return reject();
    }
    bool update(uint16_t sample, uint32_t elapsedMs) {
        if (!beginSample(elapsedMs)) return false;
        int32_t delta;
        if (!angularDelta(angle_, sample, delta)) return reject();
        if (delta < 0) {
            pending_ = false;
            count_ = 0;
            if (!suspicious_ && static_cast<uint32_t>(-delta) <= reverseTolerance_) {
                reverseActive_ = true;
                return false;
            }
            return reject();
        }
        reverseActive_ = false;
        MovementBudget candidateBudget = budget_;
        if (!candidateBudget.consume(static_cast<uint16_t>(delta))) return reject();
        if (suspicious_ || delta > 0) {
            if (!pending_ || candidate_ != sample) {
                pending_ = true;
                candidate_ = sample;
                count_ = 1;
                return false;
            }
            if (++count_ < stableSamples_) return false;
        }
        budget_ = candidateBudget;
        pending_ = false;
        count_ = 0;
        unconfirmedMs_ = 0;
        suspicious_ = false;
        angle_ = sample;
        lastDelta_ = static_cast<uint16_t>(delta);
        return true;
    }
private:
    bool beginSample(uint32_t elapsedMs) {
        lastDelta_ = 0;
        if (!valid_) return false;
        if (!healthy()) {
            const uint32_t room = timeoutMs_ - unconfirmedMs_;
            unconfirmedMs_ += elapsedMs > room ? room : elapsedMs;
            if (unconfirmedMs_ >= timeoutMs_) { invalidate(); return false; }
        }
        budget_.accrue(elapsedMs);
        return true;
    }
    bool reject() {
        pending_ = false;
        count_ = 0;
        reverseActive_ = false;
        suspicious_ = true;
        if (unconfirmedMs_ >= timeoutMs_) invalidate();
        return false;
    }
    uint16_t angle_;
    uint16_t lastDelta_;
    MovementBudget budget_;
    uint32_t timeoutMs_;
    uint32_t unconfirmedMs_;
    bool suspicious_;
    bool valid_;
    bool reverseActive_;
    bool pending_;
    uint16_t candidate_;
    uint8_t stableSamples_;
    uint8_t count_;
    uint16_t reverseTolerance_;
};
}
