#pragma once

#include <cmath>
#include <limits>

#include "f4_ballistic_solver.hpp"

namespace rm_assessment {

// Flat-earth projectile baseline. It deliberately exposes the common edge
// cases for F4: invalid inputs, no real low-angle solution, and non-finite
// arithmetic. Angles are radians and flight time is seconds.
//枚举使上层能区分“没解”和“输入非法”：前者禁火，后者报错
enum class BallisticStatus {
  Ok = 0,
  InvalidInput,     // 非有限值，或距离/高度/弹速超出物理量程
  NoSolution,       // 判别式 < 0：该弹速在此距离/高度下无实数解
  PitchOutOfRange,  // 低弹道解存在，但仰角超过云台可行范围
  ResidualTooLarge  // 数值回代误差过大，结果不可信
};
struct BallisticDiagnosis {
  BallisticResult result;
  BallisticStatus status = BallisticStatus::InvalidInput;
};
class BaselineBallisticSolver final : public BallisticSolver {
 public:
//   gravity_mps       重力加速度，m/s^2，忽略空气阻力
//   distance_m        水平距离，米
//   height_m          目标相对枪口的高度差，米，目标更高为正
//   bullet_speed_mps  弹速，米/秒
//   pitch_rad         俯仰角，弧度，向上为正
//   flight_time_sec   弹丸飞行时间，秒

 static constexpr double gravity_mps2 = 9.80665;
  static constexpr double min_distance_m = 0.5;
  static constexpr double max_distance_m = 60.0;
  static constexpr double max_abs_height_m = 12.0;
  static constexpr double min_bullet_speed_mps = 5.0;
  static constexpr double max_bullet_speed_mps = 60.0;
  // 云台俯仰可行范围
  static constexpr double max_abs_pitch_rad = 0.7853981633974483;  // 45 deg -> rad
  static constexpr double max_flight_time_sec = 2.0;
  // 回代允许的最大偏差：把解代回原方程，落点高度误差应小于 1 mm。
  static constexpr double max_residual_m = 1e-3;
  // 接口保持不变：只返回 BallisticResult，valid=false 时上层必须禁火。
  // 纯函数,避免无解沿用上一帧旧解
  BallisticResult solve(double distance_m, double height_m,
                        double bullet_speed_mps) const override {
     return solveDetailed(distance_m, height_m, bullet_speed_mps).result;
  }  
  // 带失败原因的求解，供测试使用                  
    //constexpr double gravity = 9.80665;
    BallisticDiagnosis solveDetailed(double distance_m, double height_m,
                                   double bullet_speed_mps) const {
    BallisticDiagnosis out;
    // 先做输入契约检查，NaN/Inf 和越界值一律直接判无效，不再进入后面的算术，避免 NaN 传播到云台指令。
    if (!std::isfinite(distance_m) || !std::isfinite(height_m) ||
        !std::isfinite(bullet_speed_mps)) {
      out.status = BallisticStatus::InvalidInput;
      return out;
    }
    if (distance_m < min_distance_m || distance_m > max_distance_m ||
        std::abs(height_m) > max_abs_height_m ||
        bullet_speed_mps < min_bullet_speed_mps ||
        bullet_speed_mps > max_bullet_speed_mps) {
      out.status = BallisticStatus::InvalidInput;
      return out;
    }
    //判别式单独判断，并显式区分“无解”与“输入非法”。
    //tan:v^4 - g*(g*d^2 + 2*h*v^2) >= 0。
    const double speed2 = bullet_speed_mps * bullet_speed_mps;
     const double discriminant =
        speed2 * speed2 -gravity_mps2 * (gravity_mps2 * distance_m * distance_m +2.0 * height_m * speed2);
    if (!std::isfinite(discriminant) || discriminant < 0.0){
      out.status = BallisticStatus::NoSolution;
      return out;
    }
    // 取减号分支 = 低弹道（平直）解。
    const double tan_pitch = 
        (speed2 - std::sqrt(discriminant)) / (gravity_mps2 * distance_m);
    const double pitch = std::atan(tan_pitch);
    if (!std::isfinite(pitch)) {
      out.status = BallisticStatus::NoSolution;
      return out;
    }
    // 仰角超限按“无解”处理，让上层禁火或降级
    if (std::abs(pitch) > max_abs_pitch_rad) {
      out.status = BallisticStatus::PitchOutOfRange;
      return out;
    }
    const double cos_pitch = std::cos(pitch);
    const double horizontal_speed = bullet_speed_mps * cos_pitch;
    if (!std::isfinite(horizontal_speed) || horizontal_speed <= 1e-6) {
      out.status = BallisticStatus::NoSolution;
      return out;
    }
    const double flight_time = distance_m / horizontal_speed;
    if (!std::isfinite(flight_time) || flight_time <= 0.0 ||
        flight_time > max_flight_time_sec) {
      out.status = BallisticStatus::NoSolution;
      return out;
    }
    // 正向回代自检。把解代回弹道方程，落点高度必须等于给定高度差；
    // 误差过大判为无解
    const double back_substituted_height =
        distance_m * tan_pitch -
        gravity_mps2 * distance_m * distance_m /
            (2.0 * speed2 * cos_pitch * cos_pitch);
    const double residual = std::abs(back_substituted_height - height_m);
    if (!std::isfinite(residual) || residual > max_residual_m) {
      out.status = BallisticStatus::ResidualTooLarge;
      return out;
    }

    out.result = BallisticResult{true, pitch, flight_time};
    out.status = BallisticStatus::Ok;
    return out;
  }
};

}  // namespace rm_assessment
