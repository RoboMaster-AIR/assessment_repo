#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

#include "f1_detector.hpp"

namespace rm_assessment {

// Deliberately weak starter: greedily pairs same-color bars on the right. It
// is useful for running the public replay, but fails on cross-vehicle pairs
// and partially occluded armor. Candidates should replace this policy, not
// rewrite image segmentation.
class BaselineArmorAssociator final : public ArmorAssociator {
 public:
  std::vector<ArmorPair> associate(
      const std::vector<LightbarCandidate>& candidates) override {
    std::vector<ArmorPair> result;
    std::vector<bool> used(candidates.size(), false);
    for (std::size_t i = 0; i < candidates.size(); ++i) {
      if (used[i]) continue;
      int best = -1;
      float best_distance = std::numeric_limits<float>::infinity();
      for (std::size_t j = 0; j < candidates.size(); ++j) {
        if (i == j || used[j] || candidates[j].color != candidates[i].color) continue;
        if (candidates[j].cx <= candidates[i].cx) continue;
        const float dx = candidates[j].cx - candidates[i].cx;
        const float dy = std::abs(candidates[j].cy - candidates[i].cy);
        const float length_ratio = candidates[i].length > 0.0F
                                       ? candidates[j].length / candidates[i].length
                                       : 0.0F;
        if (dy > 0.45F * std::max(candidates[i].length, candidates[j].length) ||
            length_ratio < 0.45F || length_ratio > 2.2F ||
            std::abs(candidates[i].angle_deg - candidates[j].angle_deg) > 25.0F) {
          continue;
        }
        const float distance = dx + 2.0F * dy;
        if (distance < best_distance) {
          best_distance = distance;
          best = static_cast<int>(j);
        }
      }
      if (best >= 0) {
        used[i] = used[static_cast<std::size_t>(best)] = true;
        result.push_back(ArmorPair{static_cast<int>(i), best, 0.5F, true});
      }
    }
    return result;
  }
};

}  // namespace rm_assessment
