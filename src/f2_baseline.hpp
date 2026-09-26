#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

#include "f2_tracker.hpp"

namespace rm_assessment {

// Small executable baseline. It confirms a target twice, follows the nearest
// plausible observation, predicts briefly through a gap, and then releases.
// The matching gate and state timings are intentionally imperfect.
class BaselineReacquisitionTracker final : public ReacquisitionTracker {
 public:
  TrackOutput update(double timestamp_sec,
                     const std::vector<AnonymousObservation>& observations) override {
    const AnonymousObservation* best = nullptr;
    double best_cost = std::numeric_limits<double>::infinity();
    const bool have_center = center_.has_value();
    const float predicted_x = have_center ? center_->first + velocity_.first *
                                                        static_cast<float>(timestamp_sec - last_timestamp_)
                                          : 0.0F;
    const float predicted_y = have_center ? center_->second + velocity_.second *
                                                         static_cast<float>(timestamp_sec - last_timestamp_)
                                          : 0.0F;
    for (const auto& observation : observations) {
      if (!observation.valid || observation.score < 0.35F) continue;
      const double dx = have_center ? observation.x_px - predicted_x : 0.0;
      const double dy = have_center ? observation.y_px - predicted_y : 0.0;
      const double distance = std::sqrt(dx * dx + dy * dy);
      const double gate = have_center && state_ != TrackState::Lost ?
                              (state_ == TrackState::TempLost ? 130.0 : 80.0) :
                              std::numeric_limits<double>::infinity();
      if (distance <= gate && (best == nullptr || distance < best_cost ||
                               (distance == best_cost && observation.score > best->score))) {
        best = &observation;
        best_cost = distance;
      }
    }

    const double dt = timestamp_sec > last_timestamp_ ? timestamp_sec - last_timestamp_ : 0.0;
    if (best != nullptr) {
      if (center_.has_value() && dt > 0.0) {
        velocity_.first = (best->x_px - center_->first) / static_cast<float>(dt);
        velocity_.second = (best->y_px - center_->second) / static_cast<float>(dt);
      }
      center_ = std::make_pair(best->x_px, best->y_px);
      last_timestamp_ = timestamp_sec;
      missed_frames_ = 0;
      if (state_ == TrackState::Idle || state_ == TrackState::Lost) {
        state_ = TrackState::Confirming;
        confirmation_frames_ = 1;
      } else if (state_ == TrackState::Confirming) {
        ++confirmation_frames_;
        if (confirmation_frames_ >= 2) state_ = TrackState::Tracking;
      } else {
        state_ = TrackState::Tracking;
      }
    } else if (state_ == TrackState::Tracking || state_ == TrackState::Confirming ||
               state_ == TrackState::TempLost) {
      ++missed_frames_;
      if (state_ != TrackState::Confirming && missed_frames_ <= 15) {
        state_ = TrackState::TempLost;
        if (center_.has_value()) {
          center_->first += velocity_.first * static_cast<float>(dt);
          center_->second += velocity_.second * static_cast<float>(dt);
        }
      } else if (missed_frames_ > 30) {
        state_ = TrackState::Lost;
        center_.reset();
        velocity_ = {0.0F, 0.0F};
      }
    }
    return TrackOutput{state_, center_, missed_frames_};
  }

  void reset() override {
    state_ = TrackState::Idle;
    center_.reset();
    velocity_ = {0.0F, 0.0F};
    last_timestamp_ = 0.0;
    missed_frames_ = 0;
    confirmation_frames_ = 0;
  }

 private:
  TrackState state_ = TrackState::Idle;
  std::optional<std::pair<float, float>> center_;
  std::pair<float, float> velocity_{0.0F, 0.0F};
  double last_timestamp_ = 0.0;
  int missed_frames_ = 0;
  int confirmation_frames_ = 0;
};

}  // namespace rm_assessment
