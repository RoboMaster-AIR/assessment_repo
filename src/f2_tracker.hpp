#pragma once

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace rm_assessment {

struct AnonymousObservation {
  std::int64_t frame = 0;
  double timestamp_sec = 0.0;
  float x_px = 0.0F;
  float y_px = 0.0F;
  float score = 0.0F;
  bool valid = true;
};

enum class TrackState { Idle, Confirming, Tracking, TempLost, Lost };

struct TrackOutput {
  TrackState state = TrackState::Idle;
  std::optional<std::pair<float, float>> center_px;
  int missed_frames = 0;
};

class ReacquisitionTracker {
 public:
  virtual ~ReacquisitionTracker() = default;
  virtual TrackOutput update(double timestamp_sec,
                             const std::vector<AnonymousObservation>& observations) = 0;
  virtual void reset() = 0;
};

}  // namespace rm_assessment
