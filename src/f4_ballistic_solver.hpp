#pragma once

namespace rm_assessment {

struct BallisticResult {
  bool valid = false;
  double pitch_rad = 0.0;
  double flight_time_sec = 0.0;
};

class BallisticSolver {
 public:
  virtual ~BallisticSolver() = default;
  virtual BallisticResult solve(double distance_m, double height_m,
                                double bullet_speed_mps) const = 0;
};

}  // namespace rm_assessment
