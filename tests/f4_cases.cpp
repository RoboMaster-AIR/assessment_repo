#include <cmath>
#include <cstdio>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "f4_ballistic_solver.hpp"
#include "f4_baseline.hpp"
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

inline bool ToNumber(const std::string& text, double& out) {
  try {
    std::size_t consumed = 0;
    out = std::stod(text, &consumed);
    return consumed > 0;
  } catch (const std::exception&) {
    return false;
  }
}

// 取 JSONL 行里的数值字段，例如 "threat":0.7
inline bool JsonNumber(const std::string& line, const char* key, double& out) {
  const std::regex pattern(std::string("\"") + key +
                           "\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?(?:[eE][-+]?[0-9]+)?)");
  std::smatch match;
  if (!std::regex_search(line, match, pattern)) return false;
  return ToNumber(match[1].str(), out);
}

}  // namespace rm_test

namespace {

const char* StatusName(rm_assessment::BallisticStatus status) {
  switch (status) {
    case rm_assessment::BallisticStatus::Ok: return "ok";
    case rm_assessment::BallisticStatus::InvalidInput: return "invalid_input";
    case rm_assessment::BallisticStatus::NoSolution: return "no_solution";
    case rm_assessment::BallisticStatus::PitchOutOfRange: return "pitch_out_of_range";
    case rm_assessment::BallisticStatus::ResidualTooLarge: return "residual_too_large";
  }
  return "unknown";
}

// 独立复算：把解代回弹道方程，检查落点高度是否等于给定高度差。
double Residual(const rm_assessment::BallisticResult& result, double distance_m,
                double height_m, double bullet_speed_mps) {
  if (!result.valid) return 0.0;
  const double gravity = 9.80665;
  const double cos_pitch = std::cos(result.pitch_rad);
  const double predicted_height =
      distance_m * std::tan(result.pitch_rad) -
      gravity * distance_m * distance_m /
          (2.0 * bullet_speed_mps * bullet_speed_mps * cos_pitch * cos_pitch);
  return std::fabs(predicted_height - height_m);
}

}  // namespace

int main(int argc, char** argv) {
  const std::string path = argc > 1 ? argv[1] : "data/F4/F4_ballistic_cases.jsonl";
  bool opened = false;
  const std::vector<std::string> lines = rm_test::LoadLines(path, opened);
  // 输入不可用必须直接失败：否则路径写错时会因为只跑了边界用例而报 PASS，
  // 看起来像“公开数据全部通过”，实际根本没读数据。
  if (!opened || lines.empty()) {
    std::cerr << "cannot read ballistic cases from " << path << "\n";
    return 1;
  }

  rm_assessment::BaselineBallisticSolver solver;
  long long valid_cases = 0;
  long long invalid_cases = 0;
  long long non_finite_outputs = 0;
  long long residual_violations = 0;

  {
    std::printf("PUBLIC CASES\n");
    std::printf("case,distance_m,height_m,speed_mps,valid,status,pitch_deg,"
                "flight_time_sec,residual_m\n");
    for (const std::string& line : lines) {
      double distance = 0.0;
      double height = 0.0;
      double speed = 0.0;
      if (!rm_test::JsonNumber(line, "distance_m", distance)) continue;
      rm_test::JsonNumber(line, "height_m", height);
      if (!rm_test::JsonNumber(line, "speed_mps", speed)) continue;
      double case_id = -1.0;
      rm_test::JsonNumber(line, "case", case_id);

      const rm_assessment::BallisticDiagnosis diagnosis =
          solver.solveDetailed(distance, height, speed);
      const rm_assessment::BallisticResult& result = diagnosis.result;
      if (!std::isfinite(result.pitch_rad) ||
          !std::isfinite(result.flight_time_sec)) {
        ++non_finite_outputs;
      }
      const double residual = Residual(result, distance, height, speed);
      if (result.valid) {
        ++valid_cases;
        if (residual > rm_assessment::BaselineBallisticSolver::max_residual_m) {
          ++residual_violations;
        }
        // 有效解必须满足物理约束：仰角在云台范围内，飞行时间为正。
        if (std::fabs(result.pitch_rad) >
                rm_assessment::BaselineBallisticSolver::max_abs_pitch_rad ||
            result.flight_time_sec <= 0.0) {
          ++residual_violations;
        }
      } else {
        ++invalid_cases;
        // 无解时必须给出全零结果，不能沿用任何旧角度。
        if (result.pitch_rad != 0.0 || result.flight_time_sec != 0.0) {
          ++residual_violations;
        }
      }
      std::printf("%.0f,%.3f,%.3f,%.3f,%d,%s,%.4f,%.6f,%.9f\n", case_id, distance,
                  height, speed, result.valid ? 1 : 0, StatusName(diagnosis.status),
                  result.pitch_rad * 180.0 / 3.141592653589793,
                  result.flight_time_sec, residual);
    }
  }

  // ---- 边界与畸形输入 ----
  struct Case {
    const char* name;
    double distance;
    double height;
    double speed;
    bool expect_valid;
  };
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  const std::vector<Case> boundary = {
      {"zero_distance", 0.0, 0.0, 20.0, false},
      {"negative_distance", -3.0, 0.0, 20.0, false},
      {"nan_distance", nan, 0.0, 20.0, false},
      {"inf_distance", inf, 0.0, 20.0, false},
      {"zero_speed", 2.0, 0.0, 0.0, false},
      {"negative_speed", 2.0, 0.0, -20.0, false},
      {"nan_speed", 2.0, 0.0, nan, false},
      {"nan_height", 2.0, nan, 20.0, false},
      {"huge_height", 2.0, 1000.0, 20.0, false},
      {"beyond_max_range", 1000.0, 0.0, 20.0, false},
      {"no_real_solution", 45.0, -6.0, 15.0, false},
      {"low_speed_short_range", 2.0, 0.0, 18.0, true},
      {"typical", 5.0, 0.5, 22.0, true},
      {"steep_downhill", 8.0, -3.0, 25.0, true},
  };

  std::printf("\nBOUNDARY CASES\n");
  std::printf("name,distance_m,height_m,speed_mps,valid,status,pitch_deg,"
              "flight_time_sec,as_expected\n");
  long long boundary_mismatches = 0;
  for (const Case& item : boundary) {
    const rm_assessment::BallisticDiagnosis diagnosis =
        solver.solveDetailed(item.distance, item.height, item.speed);
    const rm_assessment::BallisticResult& result = diagnosis.result;
    if (!std::isfinite(result.pitch_rad) ||
        !std::isfinite(result.flight_time_sec)) {
      ++non_finite_outputs;
    }
    const bool as_expected = result.valid == item.expect_valid;
    if (!as_expected) ++boundary_mismatches;
    std::printf("%s,%.3f,%.3f,%.3f,%d,%s,%.4f,%.6f,%s\n", item.name, item.distance,
                item.height, item.speed, result.valid ? 1 : 0,
                StatusName(diagnosis.status),
                result.pitch_rad * 180.0 / 3.141592653589793,
                result.flight_time_sec, as_expected ? "yes" : "NO");
  }

  // ---- 无解不能沿用旧解 ----
  // 先算一个有效解，再算一个无解案例，两者结果必须不同且后者为全零。
  const rm_assessment::BallisticResult valid_result = solver.solve(2.0, 0.0, 20.0);
  const rm_assessment::BallisticResult invalid_result = solver.solve(45.0, -6.0, 15.0);
  const bool stale_reuse =
      invalid_result.valid ||
      (invalid_result.pitch_rad == valid_result.pitch_rad &&
       invalid_result.flight_time_sec == valid_result.flight_time_sec);

  // ---- 可复现性 ----
  const rm_assessment::BallisticResult repeat_a = solver.solve(5.0, 0.5, 22.0);
  const rm_assessment::BallisticResult repeat_b = solver.solve(5.0, 0.5, 22.0);
  const bool reproducible =
      repeat_a.valid == repeat_b.valid &&
      repeat_a.pitch_rad == repeat_b.pitch_rad &&
      repeat_a.flight_time_sec == repeat_b.flight_time_sec;

  std::printf("\nSUMMARY\n");
  std::printf("  public_valid=%lld public_invalid=%lld\n", valid_cases,
              invalid_cases);
  std::printf("  non_finite_outputs=%lld residual_violations=%lld "
              "boundary_mismatches=%lld\n",
              non_finite_outputs, residual_violations, boundary_mismatches);
  std::printf("  no_solution_reuses_previous=%s reproducible=%s\n",
              stale_reuse ? "yes" : "no", reproducible ? "yes" : "no");

  if (non_finite_outputs > 0) {
    std::cerr << "FAIL: " << non_finite_outputs
              << " output(s) are NaN/Inf\n";
    return 2;
  }
  if (residual_violations > 0 || boundary_mismatches > 0) {
    std::cerr << "FAIL: residual or boundary expectation violated\n";
    return 3;
  }
  if (stale_reuse) {
    std::cerr << "FAIL: a no-solution case reused a previous solution\n";
    return 4;
  }
  if (!reproducible) {
    std::cerr << "FAIL: solver is not reproducible\n";
    return 5;
  }
  std::printf("RESULT: PASS\n");
  return 0;
}
