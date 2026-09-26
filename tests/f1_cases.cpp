#include <cmath>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "f1_baseline.hpp"
#include "f1_detector.hpp"
#include <fstream>
#include <regex>
#include <stdexcept>

namespace rm_test {

// 逐行读取文本文件。opened 反映文件是否成功打开。
inline std::vector<std::string> LoadLines(const std::string& path, bool& opened) {
  std::vector<std::string> lines;
  std::ifstream input(path);
  opened = static_cast<bool>(input);
  if (!opened) return lines;
  std::string line;
  while (std::getline(input, line)) {
    // 兼容 CRLF：去掉行尾的回车，避免字段值里混入 '\r'。
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
      line.pop_back();
    }
    if (!line.empty()) lines.push_back(line);
  }
  return lines;
}

// 按逗号切分 CSV 行（公开数据不含带逗号的引号字段）。
inline std::vector<std::string> SplitCsv(const std::string& line) {
  std::vector<std::string> fields;
  std::string current;
  for (const char c : line) {
    if (c == ',') {
      fields.push_back(current);
      current.clear();
      continue;
    }
    current.push_back(c);
  }
  fields.push_back(current);
  return fields;
}

inline bool ToNumber(const std::string& text, double& out) {
  try {
    std::size_t consumed = 0;
    out = std::stod(text, &consumed);
    return consumed > 0;
  } catch (const std::exception&) {
    return false;
  }
}

}  // namespace rm_test

namespace {

struct FrameData {
  std::vector<rm_assessment::LightbarCandidate> candidates;
  std::vector<std::string> sides;  // 与 candidates 一一对应，仅用于校验
};

}  // namespace

int main(int argc, char** argv) {
  const std::string path =
      argc > 1 ? argv[1] : "data/F1/F1_lightbar_candidates.csv";
  bool opened = false;
  const std::vector<std::string> lines = rm_test::LoadLines(path, opened);
  if (!opened || lines.size() < 2) {
    std::cerr << "cannot read lightbar candidates from " << path << "\n";
    return 1;
  }

  std::map<long long, FrameData> frames;
  long long parsed_rows = 0;
  for (std::size_t i = 1; i < lines.size(); ++i) {
    const std::vector<std::string> fields = rm_test::SplitCsv(lines[i]);
    if (fields.size() < 9) continue;  // 缺列的行直接跳过
    double frame = 0.0;
    double timestamp = 0.0;
    double cx = 0.0;
    double cy = 0.0;
    double length = 0.0;
    double angle = 0.0;
    double brightness = 0.0;
    double color = 0.0;
    if (!rm_test::ToNumber(fields[0], frame) ||
        !rm_test::ToNumber(fields[1], timestamp) ||
        !rm_test::ToNumber(fields[3], cx) || !rm_test::ToNumber(fields[4], cy) ||
        !rm_test::ToNumber(fields[5], length) ||
        !rm_test::ToNumber(fields[6], angle) ||
        !rm_test::ToNumber(fields[7], brightness) ||
        !rm_test::ToNumber(fields[8], color)) {
      continue;
    }
    rm_assessment::LightbarCandidate candidate;
    candidate.frame = static_cast<std::int64_t>(frame);
    candidate.timestamp_sec = timestamp;
    candidate.cx = static_cast<float>(cx);
    candidate.cy = static_cast<float>(cy);
    candidate.length = static_cast<float>(length);
    candidate.angle_deg = static_cast<float>(angle);
    candidate.brightness = static_cast<float>(brightness);
    candidate.color = static_cast<std::uint8_t>(color);
    FrameData& data = frames[candidate.frame];
    data.candidates.push_back(candidate);
    data.sides.push_back(fields[2]);
    ++parsed_rows;
  }
  if (parsed_rows == 0) {
    std::cerr << "no parsable rows in " << path << "\n";
    return 1;
  }

  rm_assessment::BaselineArmorAssociator associator;
  std::printf("frame,left_index,right_index,complete,confidence\n");

  long long total_pairs = 0;
  long long complete_pairs = 0;
  long long same_side_pairs = 0;
  long long oversized_pairs = 0;
  long long wide_but_allowed = 0;
  long long frames_with_leftover = 0;
  long long paired_bars = 0;
  long long candidate_bars = 0;
  long long noise_bars = 0;
  long long non_deterministic = 0;

  for (const auto& entry : frames) {
    const FrameData& data = entry.second;
    const std::vector<rm_assessment::ArmorPair> pairs =
        associator.associate(data.candidates);
    // 可复现性：同一帧重复调用必须给出完全相同的输出。
    const std::vector<rm_assessment::ArmorPair> repeated =
        associator.associate(data.candidates);
    if (pairs.size() != repeated.size()) {
      ++non_deterministic;
    } else {
      for (std::size_t i = 0; i < pairs.size(); ++i) {
        if (pairs[i].left_index != repeated[i].left_index ||
            pairs[i].right_index != repeated[i].right_index ||
            pairs[i].complete != repeated[i].complete ||
            pairs[i].confidence != repeated[i].confidence) {
          ++non_deterministic;
          break;
        }
      }
    }

    for (const std::string& side : data.sides) {
      if (side == "distractor") ++noise_bars;
      ++candidate_bars;
    }

    std::vector<char> matched(data.candidates.size(), 0);
    for (const rm_assessment::ArmorPair& pair : pairs) {
      const std::size_t left = static_cast<std::size_t>(pair.left_index);
      const std::size_t right = static_cast<std::size_t>(pair.right_index);
      if (left >= data.candidates.size() || right >= data.candidates.size()) {
        continue;
      }
      matched[left] = 1;
      matched[right] = 1;
      ++total_pairs;
      paired_bars += 2;
      if (pair.complete) ++complete_pairs;

      // 独立校验 1：同一块装甲板必须是“左侧灯条 + 右侧灯条”，
      // 两侧标签相同说明这里发生了误配。
      if (data.sides[left] == data.sides[right]) ++same_side_pairs;

      // 独立校验 2：宽度/长度比超出算法上限说明内部约束失效。
      const float mean_length = 0.5F * (data.candidates[left].length +
                                        data.candidates[right].length);
      const float width_ratio = mean_length > 0.0F
                                    ? (data.candidates[right].cx -
                                       data.candidates[left].cx) / mean_length
                                    : 0.0F;
      if (width_ratio > rm_assessment::BaselineArmorAssociator::max_width_length_ratio) {
        ++oversized_pairs;
      } else if (width_ratio > 2.75F) {
        // 公开数据里真实装甲板的宽度比不超过 2.74，超过即需要人工复核。
        ++wide_but_allowed;
      }
      std::printf("%lld,%d,%d,%d,%.3f\n", entry.first, pair.left_index,
                  pair.right_index, pair.complete ? 1 : 0,
                  static_cast<double>(pair.confidence));
    }
    for (std::size_t i = 0; i < matched.size(); ++i) {
      if (!matched[i]) {
        ++frames_with_leftover;
        break;
      }
    }
  }

  std::printf("SUMMARY\n");
  std::printf("  frames=%zu rows=%lld bars=%lld noise_bars=%lld\n", frames.size(),
              parsed_rows, candidate_bars, noise_bars);
  std::printf("  pairs=%lld complete=%lld incomplete=%lld\n", total_pairs,
              complete_pairs, total_pairs - complete_pairs);
  std::printf("  paired_bars=%lld bars_left_unmatched=%lld\n", paired_bars,
              candidate_bars - noise_bars - paired_bars);
  std::printf("  frames_with_unmatched_bar=%lld\n", frames_with_leftover);
  std::printf("  same_side_pairs=%lld oversized_pairs=%lld "
              "wide_but_allowed=%lld non_deterministic=%lld\n",
              same_side_pairs, oversized_pairs, wide_but_allowed,
              non_deterministic);

  if (total_pairs == 0) {
    std::cerr << "FAIL: no armor pair produced\n";
    return 2;
  }
  if (same_side_pairs > 0) {
    std::cerr << "FAIL: " << same_side_pairs
              << " pair(s) joined two bars with the same side label\n";
    return 3;
  }
  if (oversized_pairs > 0) {
    std::cerr << "FAIL: " << oversized_pairs
              << " pair(s) exceed the width cap (internal constraint broken)\n";
    return 4;
  }
  if (non_deterministic > 0) {
    std::cerr << "FAIL: association is not reproducible on " << non_deterministic
              << " frame(s)\n";
    return 5;
  }
  std::printf("RESULT: PASS\n");
  return 0;
}
