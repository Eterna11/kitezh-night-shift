#include "kitezh/lock.hpp"
#include "kitezh/geometry.hpp"

namespace kitezh {

Lock::Lock() = default;

Level Lock::level() const {
    return level_;
}

GateState Lock::gate(Gate g) const {
    return (g == Gate::Lower) ? lower_gate_ : upper_gate_;
}

int Lock::pumpCycles() const {
    return pump_cycles_;
}

std::vector<Vessel> Lock::inside() const {
    return inside_;
}

void Lock::open(Gate g) {
    if (gate(g) == GateState::Open) {
        return;
    }

    if (g == Gate::Lower && level_ != Level::Low) {
        throw LockError("Cannot open lower gate at high level");
    }
    if (g == Gate::Upper && level_ != Level::High) {
        throw LockError("Cannot open upper gate at low level");
    }

    if (lower_gate_ == GateState::Open || upper_gate_ == GateState::Open) {
        throw LockError("Both gates cannot be open at the same time");
    }

    if (g == Gate::Lower) {
        lower_gate_ = GateState::Open;
    } else {
        upper_gate_ = GateState::Open;
    }
}

void Lock::close(Gate g) {
    if (g == Gate::Lower) {
        lower_gate_ = GateState::Closed;
    } else {
        upper_gate_ = GateState::Closed;
    }
}

void Lock::pump() {
    if (lower_gate_ != GateState::Closed || upper_gate_ != GateState::Closed) {
        throw LockError("Cannot pump with open gates");
    }

    Level next_level = (level_ == Level::Low) ? Level::High : Level::Low;
    int next_depth = (next_level == Level::Low) ? kLowDepthDm : kHighDepthDm;

    for (const Vessel& v : inside_) {
        if (v.draft_dm > next_depth) {
            throw LockError("Vessel draft exceeds depth of the target level");
        }
    }

    level_ = next_level;
    ++pump_cycles_;
}

void Lock::enter(const Vessel& vessel) {
    if (validateVessel(vessel).has_value()) {
        throw LockError("Invalid vessel");
    }

    for (const Vessel& v : inside_) {
        if (v.call_sign == vessel.call_sign) {
            throw LockError("Duplicate call sign inside chamber");
        }
    }

    int current_depth = (level_ == Level::Low) ? kLowDepthDm : kHighDepthDm;
    if (vessel.draft_dm > current_depth) {
        throw LockError("Vessel draft exceeds current chamber depth");
    }

    Gate entry_gate = (vessel.direction == Direction::Upstream) ? Gate::Lower : Gate::Upper;
    if (gate(entry_gate) != GateState::Open) {
        throw LockError("Entry gate is not open");
    }

    if (vessel.kind == VesselKind::Oversized && !inside_.empty()) {
        throw LockError("Oversized vessel can only enter an empty chamber");
    }
    for (const Vessel& v : inside_) {
        if (v.kind == VesselKind::Oversized) {
            throw LockError("Cannot enter chamber while an oversized vessel is inside");
        }
    }

    if (!fitsInChamber(inside_, vessel)) {
        throw LockError("Vessel does not fit in chamber by length");
    }

    inside_.push_back(vessel);
}

void Lock::exit(const std::string& call_sign) {
    if (inside_.empty() || inside_.front().call_sign != call_sign) {
        throw LockError("Vessel is not at the front of the chamber queue");
    }

    const Vessel& front_vessel = inside_.front();

    Gate exit_gate = (front_vessel.direction == Direction::Upstream) ? Gate::Upper : Gate::Lower;
    if (gate(exit_gate) != GateState::Open) {
        throw LockError("Exit gate is not open");
    }

    inside_.erase(inside_.begin());
}

}  // namespace kitezh