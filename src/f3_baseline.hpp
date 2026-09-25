#pragma once

#include <algorithm>
#include <limits>

#include "f3_target_manager.hpp"

namespace rm_assessment {

// Deliberately twitchy baseline: after filtering unusable/hostile entries it
// chooses the highest threat on every call. F3 candidates add switch
// hysteresis, temporary-loss handling, and command validation.
class BaselineTargetManager final : public TargetManager {
 public:
  TargetDecision update(double timestamp_ms,
                        const std::vector<TargetSnapshot>& targets,
                        const std::optional<std::string>& operator_command) override {
    const TargetSnapshot* best = nullptr;
    for (const auto& target : targets) {
      if (!target.is_enemy || !target.visible || target.age_ms > 200) continue;
      if (best == nullptr || target.threat > best->threat) best = &target;
    }
    if (operator_command.has_value() && operator_command->rfind("lock:", 0) == 0) {
      const std::string requested = operator_command->substr(5);
      for (const auto& target : targets) {
        if (target.id == requested && target.is_enemy && target.visible && target.age_ms <= 200) {
          best = &target;
          break;
        }
      }
    }
    if (best == nullptr) {
      current_id_.reset();
      return TargetDecision{std::nullopt, false, "IDLE", "no usable enemy"};
    }
    current_id_ = best->id;
    (void)timestamp_ms;
    return TargetDecision{current_id_, true, "TRACKING", "highest threat baseline"};
  }

  void reset() override { current_id_.reset(); }

 private:
  std::optional<std::string> current_id_;
};

}  // namespace rm_assessment
