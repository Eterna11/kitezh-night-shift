#include "kitezh/shift.hpp"

#include "kitezh/geometry.hpp"
#include "kitezh/lock.hpp"

#include <unordered_set>
#include <vector>

namespace kitezh {

ShiftReport runNightShift(const std::vector<Vessel>& queue) {
    ShiftReport report;
    std::unordered_set<std::string> seen_call_signs;
    std::vector<Vessel> waiting;

    for (const Vessel& v : queue) {
        if (seen_call_signs.count(v.call_sign) > 0) {
            report.refused.push_back({v.call_sign, "duplicate"});
            continue;
        }
        seen_call_signs.insert(v.call_sign);

        if (validateVessel(v).has_value()) {
            report.refused.push_back({v.call_sign, "invalid"});
            continue;
        }

        if (v.draft_dm > kLowDepthDm) {
            report.refused.push_back({v.call_sign, "draft"});
            continue;
        }

        waiting.push_back(v);
    }

    Lock lock;

    while (!waiting.empty()) {
        std::vector<Vessel> batch;
        std::vector<Vessel> next_waiting;

        Vessel first = waiting.front();
        batch.push_back(first);
        Direction dir = first.direction;

        if (first.kind == VesselKind::Oversized) {
            for (size_t i = 1; i < waiting.size(); ++i) {
                next_waiting.push_back(waiting[i]);
            }
        } else {
            bool skipped = false;
            for (size_t i = 1; i < waiting.size(); ++i) {
                const Vessel& candidate = waiting[i];
                bool can_take = (candidate.direction == dir) &&
                                (candidate.kind != VesselKind::Oversized) &&
                                fitsInChamber(batch, candidate) &&
                                (!skipped || candidate.kind == VesselKind::Ambulance);

                if (can_take) {
                    batch.push_back(candidate);
                } else {
                    skipped = true;
                    next_waiting.push_back(candidate);
                }
            }
        }

        waiting = std::move(next_waiting);

        Level entry_level = (dir == Direction::Upstream) ? Level::Low : Level::High;
        Gate entry_gate = (dir == Direction::Upstream) ? Gate::Lower : Gate::Upper;
        Gate exit_gate = (dir == Direction::Upstream) ? Gate::Upper : Gate::Lower;

        if (lock.level() != entry_level) {
            lock.pump();
        }

        lock.open(entry_gate);
        for (const Vessel& v : batch) {
            lock.enter(v);
        }
        lock.close(entry_gate);

        lock.pump();

        lock.open(exit_gate);
        for (const Vessel& v : batch) {
            lock.exit(v.call_sign);
            report.passed.push_back(v.call_sign);
        }
        lock.close(exit_gate);
    }

    report.pump_cycles = lock.pumpCycles();
    return report;
}

}  // namespace kitezh