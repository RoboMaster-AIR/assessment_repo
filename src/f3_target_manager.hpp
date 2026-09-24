#pragma once

#include <optional>
#include <string>
#include <vector>

namespace rm_assessment {

struct TargetSnapshot {
  std::string id;
  double distance_m = 0.0;
  double bearing_rad = 0.0;
  double threat = 0.0;
  bool is_enemy = false;
  bool visible = true;
  bool operator_priority = false;
  int age_ms = 0;
  int track_age_frames = 0;
};

struct TargetDecision {
  std::optional<std::string> selected_id;
  bool fire_enable = false;
  std::string state;
  std::string reason;
};

class TargetManager {
 public:
  virtual ~TargetManager() = default;
  virtual TargetDecision update(double timestamp_ms,
                                const std::vector<TargetSnapshot>& targets,
                                const std::optional<std::string>& operator_command) = 0;
  virtual void reset() = 0;
};

// A runnable but deliberately twitchy policy is provided in
// f3_baseline.hpp. F3 is about replacing that policy with hysteresis and safe
// release; it is not a request to build a detector or tracker.

}  // namespace rm_assessment
