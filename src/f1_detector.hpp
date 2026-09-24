#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <vector>

namespace rm_assessment {

struct ImageFrame {
  std::int64_t frame = 0;
  double timestamp_sec = 0.0;
  int width = 0;
  int height = 0;
  // BGR, row-major, owned by the caller.
  std::vector<std::uint8_t> bgr;
};

struct ArmorDetection {
  std::array<float, 8> corners_xy{};  // LT, LB, RB, RT
  float confidence = 0.0F;
  bool complete = false;
};

// F1: implement this interface without depending on air_vision_27.
class ArmorDetector {
 public:
  virtual ~ArmorDetector() = default;
  virtual std::vector<ArmorDetection> detect(const ImageFrame& frame) = 0;
};

}  // namespace rm_assessment
