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

// See f4_baseline.hpp for the minimal closed-form implementation used by the
// public replay. Candidates may replace only the edge-case policy and tests.

}  // namespace rm_assessment
