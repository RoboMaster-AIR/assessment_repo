#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace rm_assessment {

struct LightbarCandidate {
  std::int64_t frame = 0;
  double timestamp_sec = 0.0;
  float cx = 0.0F;
  float cy = 0.0F;
  float length = 0.0F;
  float angle_deg = 0.0F;
  float brightness = 0.0F;
  std::uint8_t color = 0;  // 0=red, 1=blue
};

struct ArmorPair {
  int left_index = -1;
  int right_index = -1;
  float confidence = 0.0F;
  bool complete = false;  // both bars survive the visibility/geometry checks
};

// F1 operates after the supplied lightbar extractor. Candidates are anonymous;
// no target/armor truth ID is present in the public input. The candidate does
// not need to implement image decoding, BGR/HSV segmentation, or a neural
// detector; the public CSV is the detector-to-associator contract.
class ArmorAssociator {
 public:
  virtual ~ArmorAssociator() = default;
  virtual std::vector<ArmorPair> associate(
      const std::vector<LightbarCandidate>& candidates) = 0;
};

}  // namespace rm_assessment
