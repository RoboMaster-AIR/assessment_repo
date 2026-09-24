#include "f1_detector.hpp"
#include "f1_baseline.hpp"
#include "f2_tracker.hpp"
#include "f2_baseline.hpp"
#include "f3_target_manager.hpp"
#include "f3_baseline.hpp"
#include "f4_ballistic_solver.hpp"
#include "f4_baseline.hpp"
#include "f5_latency_predictor.hpp"
#include "f5_baseline.hpp"
#include "f6_replay_analyzer.hpp"
#include "f6_baseline.hpp"
#include <vector>

int main() {
  rm_assessment::BaselineArmorAssociator f1;
  rm_assessment::BaselineReacquisitionTracker f2;
  rm_assessment::BaselineTargetManager f3;
  rm_assessment::BaselineBallisticSolver f4;
  rm_assessment::BaselineLatencyPredictor f5;
  rm_assessment::BaselineReplayAnalyzer f6;
  (void)f1; (void)f2; (void)f3; (void)f4; (void)f5; (void)f6;
  const std::vector<rm_assessment::LightbarCandidate> bars = {
      {0, 0.0, 100.0F, 200.0F, 30.0F, 90.0F, 0.9F, 0},
      {0, 0.0, 140.0F, 201.0F, 29.0F, 89.0F, 0.9F, 0},
  };
  if (f1.associate(bars).empty()) return 1;
  if (f2.update(0.0, {{0, 0.0, 100.0F, 200.0F, 0.9F, true}}).center_px == std::nullopt) return 2;
  if (!f4.solve(2.0, 0.0, 20.0).valid) return 3;
  const auto prediction = f5.predict({1.0, 0.9, 100.0, 50.0, 0.05, 0.1});
  if (prediction.stale || prediction.predicted_x_px <= 100.0) return 4;
  return 0;
}
