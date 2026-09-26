#pragma once

#include <fstream>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

#include "f6_replay_analyzer.hpp"

namespace rm_assessment {

class BaselineReplayAnalyzer final : public ReplayAnalyzer {
 public:
  std::vector<ReplayIssue> analyze(const std::string& detections_path,
                                   const std::string& tracking_path,
                                   const std::string& commands_path) override {
    std::vector<ReplayIssue> issues;
    CheckTimestamps(detections_path, "detections", issues);
    CheckTimestamps(tracking_path, "tracking", issues);
    CheckTimestamps(commands_path, "commands", issues);

    std::unordered_map<long long, std::string> states;
    std::unordered_map<long long, bool> fire;
    ReadTracking(tracking_path, states);
    ReadCommands(commands_path, fire);
    for (const auto& [frame, enabled] : fire) {
      const auto state = states.find(frame);
      if (enabled && (state == states.end() || state->second != "TRACK")) {
        issues.push_back(ReplayIssue{static_cast<double>(frame),
                                     "fire_while_untracked",
                                     "fire_enable is true while tracking state is absent or not TRACK",
                                     true});
      }
    }
    return issues;
  }

 private:
  static bool ReadNumber(const std::string& line, const char* key, double& value) {
    const std::regex pattern(std::string("\\\"") + key + "\\\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
    std::smatch match;
    if (!std::regex_search(line, match, pattern)) return false;
    value = std::stod(match[1].str());
    return true;
  }

  static bool ReadLong(const std::string& line, const char* key, long long& value) {
    double number = 0.0;
    if (!ReadNumber(line, key, number)) return false;
    value = static_cast<long long>(number);
    return true;
  }

  static void CheckTimestamps(const std::string& path, const char* module,
                              std::vector<ReplayIssue>& issues) {
    std::ifstream input(path);
    if (!input) {
      issues.push_back(ReplayIssue{0.0, "missing_log", std::string(module) + " log cannot be opened", true});
      return;
    }
    std::string line;
    double previous = -1.0;
    while (std::getline(input, line)) {
      double timestamp = 0.0;
      if (!ReadNumber(line, "timestamp", timestamp)) continue;
      if (previous >= 0.0 && timestamp < previous) {
        issues.push_back(ReplayIssue{timestamp, "timestamp_reversed",
                                     std::string(module) + " timestamp is earlier than the previous record", true});
      }
      if (previous >= 0.0 && timestamp - previous > 0.05) {
        issues.push_back(ReplayIssue{timestamp, "dropped_interval",
                                     std::string(module) + " has a gap larger than 50 ms", false});
      }
      previous = timestamp;
    }
  }

  static void ReadTracking(const std::string& path,
                           std::unordered_map<long long, std::string>& states) {
    std::ifstream input(path);
    std::string line;
    const std::regex state_pattern("\\\"state\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"");
    while (std::getline(input, line)) {
      long long frame = 0;
      std::smatch match;
      if (ReadLong(line, "frame_id", frame) && std::regex_search(line, match, state_pattern)) {
        states[frame] = match[1].str();
      }
    }
  }

  static void ReadCommands(const std::string& path,
                           std::unordered_map<long long, bool>& fire) {
    std::ifstream input(path);
    std::string line;
    while (std::getline(input, line)) {
      long long frame = 0;
      double value = 0.0;
      if (ReadLong(line, "frame_id", frame) && ReadNumber(line, "fire_enable", value)) {
        fire[frame] = value > 0.0;
      }
    }
  }
};

}  // namespace rm_assessment
