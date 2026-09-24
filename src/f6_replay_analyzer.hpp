#pragma once

#include <string>
#include <vector>

namespace rm_assessment {

struct ReplayIssue {
  double timestamp_sec = 0.0;
  std::string type;
  std::string evidence;
  bool safety_relevant = false;
};

class ReplayAnalyzer {
 public:
  virtual ~ReplayAnalyzer() = default;
  virtual std::vector<ReplayIssue> analyze(const std::string& detections_path,
                                           const std::string& tracking_path,
                                           const std::string& commands_path) = 0;
};

}  // namespace rm_assessment
