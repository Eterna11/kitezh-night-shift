#include "kitezh/vessel.hpp"
#include "kitezh/geometry.hpp"

namespace kitezh {

std::optional<std::string> validateVessel(const Vessel& vessel) {
    if (vessel.call_sign.empty()) {
        return "empty call sign";
    }

    for (char c : vessel.call_sign) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            return "bad call sign";
        }
    }

    if (vessel.length_m <= 0 || vessel.length_m > kChamberLengthM) {
        return "bad length";
    }

    if (vessel.draft_dm <= 0) {
        return "bad draft";
    }

    return std::nullopt;
}

bool fitsInChamber(const std::vector<Vessel>& inside, const Vessel& next) {
    int total_length = next.length_m;

    for (const Vessel& v : inside) {
        total_length += v.length_m + kGapM;
    }

    return total_length <= kChamberLengthM;
}

}  // namespace kitezh