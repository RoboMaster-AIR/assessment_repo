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

}  // namespace rm_assessment
