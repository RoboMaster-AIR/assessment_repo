#pragma once

namespace rm_assessment {

struct LatencySample {
  double now_sec = 0.0;
  double observed_timestamp_sec = 0.0;
  double observed_x_px = 0.0;
  double estimated_velocity_pxps = 0.0;
  double processing_delay_sec = 0.0;
  double flight_time_sec = 0.0;
};

struct Prediction {
  bool stale = false;
  double predict_horizon_sec = 0.0;
  double predicted_x_px = 0.0;
};

class LatencyPredictor {
 public:
  virtual ~LatencyPredictor() = default;
  virtual Prediction predict(const LatencySample& sample) const = 0;
};

// f5_baseline.hpp contains a runnable one-line constant-velocity baseline.
// The task is to correct its time semantics and stale handling, not to create
// a camera, serial driver, or projectile model.

}  // namespace rm_assessment
