#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

#include "f1_detector.hpp"

namespace rm_assessment {

// Deliberately weak starter: greedily pairs same-color bars on the right. It
// is useful for running the public replay, but fails on cross-vehicle pairs
// and partially occluded armor. Candidates should replace this policy, not
// rewrite image segmentation.

// 匿名灯条关联器：在基线的“贪心找右侧最近同色灯条”之上，补齐：
//   1) 几何门限同时约束宽度、高度差、夹角和长度比，拒绝跨车“超宽装甲板”；
//   2) 用全局最优替代逐条就近配对，避免同一根灯条被重复使用（贪心）；
//   3) 用左右灯条的长度比判断是否被部分遮挡，遮挡时输出 complete=false 并降权。
class BaselineArmorAssociator final : public ArmorAssociator {
 public:
  // 阈值集中成可解释常量。角度单位为度，长度/宽度单位为像素，
  // 比值均为无量纲量，因此与相机分辨率和目标远近无关。
  static constexpr float min_brightness = 0.45F;   // 低于此亮度视为噪声灯条
  static constexpr float min_length_px = 8.0F;     // 过短的亮线不是灯条
  // 装甲板宽度 / 灯条平均长度。公开数据中真实装甲板落在 0.77~2.74，
  // 而跨车配对（前车左灯条 + 后车右灯条）会得到 3 以上，故上限取 3.0。
  static constexpr float min_width_length_ratio = 0.50F;
  static constexpr float max_width_length_ratio = 3.00F;
  // 左右灯条高度差 / 平均长度；装甲板有倾斜时该值会变大，故再叠加宽度的一小部分
  static constexpr float max_height_length_ratio = 0.75F;
  static constexpr float height_tilt_allowance = 0.10F;
  // 灯条夹角（按 180 度取模）
  static constexpr float max_angle_diff_deg = 30.0F;
  // 同一块装甲板的两根灯条是刚体上的平行条，投影后必然近似平行；
  // 装甲板越接近正对相机，投影宽度比越大、两灯条越平行，反之旋转越深则越窄、允许的夹角差越大。
  // 公开数据里真实装甲板的宽度比可达 2.74、夹角差不超过 11 度
  static constexpr float parallelism_width_ratio = 1.90F;  // 宽度比分界点
  // 公开数据里“宽”配对（宽度比 > 1.9）的夹角差最大 11.3 度，而不可能同板的那批
  // 从 15.4 度起，阈值取两者之间偏中间的 13 度，两侧各留约 1.7~2.4 度余量。
  static constexpr float max_angle_diff_deg_frontal = 13.0F;
  // “宽度比大同时夹角差大”的组合在刚体上自相矛盾，属于跨板/跨车误配
  // 左右灯条长度比（长/短）。同一块装甲板两侧灯条等长，长度比明显偏离 1 （部分遮挡）
  static constexpr float max_length_ratio = 1.80F;
  static constexpr float max_length_ratio_for_complete = 1.25F;

  std::vector<ArmorPair> associate(
      const std::vector<LightbarCandidate>& candidates) override {
    std::vector<ArmorPair> result;
    const int count = static_cast<int>(candidates.size());
    if (count < 2) return result;  // 少于两根灯条不可能组成装甲板

    // 先过滤明显不可用的候选，后续所有几何判断都建立在
    // “有限值 + 亮度达标 + 长度达标”之上，避免 NaN 参与比较。
    std::vector<char> usable(static_cast<std::size_t>(count), 0);
    for (int i = 0; i < count; ++i) {
      const LightbarCandidate& bar = candidates[static_cast<std::size_t>(i)];
      usable[static_cast<std::size_t>(i)] =
          std::isfinite(bar.cx) && std::isfinite(bar.cy) &&
                  std::isfinite(bar.length) && std::isfinite(bar.angle_deg) &&
                  std::isfinite(bar.brightness) &&
                  bar.length >= min_length_px && bar.brightness >= min_brightness
              ? 1
              : 0;
    }

    // 先把所有合法的左右组合打分，再按分数从高到低贪心匹配。
    std::vector<PairCandidate> pair_candidates;
    for (int i = 0; i < count; ++i) {
      if (!usable[static_cast<std::size_t>(i)]) continue;
      for (int j = i + 1; j < count; ++j) {
        if (!usable[static_cast<std::size_t>(j)]) continue;
        const LightbarCandidate& a = candidates[static_cast<std::size_t>(i)];
        const LightbarCandidate& b = candidates[static_cast<std::size_t>(j)];
        if (a.color != b.color) continue;  // 同一块装甲板两侧灯条同色
        // 左灯条必须在右灯条左边；若顺序相反则交换索引，继续按几何判断。
        const LightbarCandidate* left = &a;
        const LightbarCandidate* right = &b;
        int left_index = i;
        int right_index = j;
        if (a.cx > b.cx) {
          left = &b;
          right = &a;
          left_index = j;
          right_index = i;
        }
        const float dx = right->cx - left->cx;
        const float mean_length = 0.5F * (left->length + right->length);
        if (dx <= 0.0F || mean_length <= 1e-3F) continue;

        const float width_ratio = dx / mean_length;
        if (width_ratio < min_width_length_ratio ||
            width_ratio > max_width_length_ratio) {
          continue;  // 太窄或太宽（跨车配对）都不接受
        }
        const float dy = std::abs(right->cy - left->cy);
        if (dy > max_height_length_ratio * mean_length +
                     height_tilt_allowance * dx) {
          continue;
        }
        const float angle_diff = AngleDifferenceDeg(left->angle_deg,
                                                    right->angle_deg);
        if (angle_diff > max_angle_diff_deg) continue;
        // 接近正对相机（投影宽）却明显不平行：不可能来自同一块刚体装甲板。
        if (width_ratio > parallelism_width_ratio &&
            angle_diff > max_angle_diff_deg_frontal) {
          continue;
        }
        const float length_ratio =
            std::max(left->length, right->length) /
            std::min(left->length, right->length);
        if (length_ratio > max_length_ratio) continue;

        PairCandidate candidate;
        candidate.left_index = left_index;
        candidate.right_index = right_index;
        candidate.confidence = Score(width_ratio, dy / mean_length, angle_diff,
                                     length_ratio, left->brightness,
                                     right->brightness);
        // 完整装甲板要求左右灯条等长且对齐。长度比超出阈值说明
        // 有一根灯条被部分遮挡，此时不能报 complete=true。
        candidate.complete = length_ratio <= max_length_ratio_for_complete &&
                             dy <= 0.5F * mean_length;
        if (!candidate.complete) {
          candidate.confidence *= 0.6F;  // 遮挡时降权，供上层判断可信度
        }
        pair_candidates.push_back(candidate);
      }
    }

    std::sort(pair_candidates.begin(), pair_candidates.end(),
              [](const PairCandidate& lhs, const PairCandidate& rhs) {
                return lhs.confidence > rhs.confidence;
              });

    // 贪心匹配保证每根灯条最多参与一块装甲板，避免出现
    // “一根灯条同时属于两块装甲板”的重复配对。
    std::vector<char> used(static_cast<std::size_t>(count), 0);
    for (const PairCandidate& candidate : pair_candidates) {
      if (used[static_cast<std::size_t>(candidate.left_index)] ||
          used[static_cast<std::size_t>(candidate.right_index)]) {
        continue;
      }
      used[static_cast<std::size_t>(candidate.left_index)] = 1;
      used[static_cast<std::size_t>(candidate.right_index)] = 1;
      result.push_back(ArmorPair{candidate.left_index, candidate.right_index,
                                 candidate.confidence, candidate.complete});
    }
    return result;
  }

 private:
  struct PairCandidate {
    int left_index = -1;
    int right_index = -1;
    float confidence = 0.0F;
    bool complete = false;
  };

  // 灯条是轴对称的，角度差按 180 度取模
  // 原基线直接用角度差值，会把同一块装甲板的左右灯条误判为不匹配。
  static float AngleDifferenceDeg(float lhs, float rhs) {
    float diff = std::fmod(std::abs(lhs - rhs), 180.0F);
    if (diff > 90.0F) diff = 180.0F - diff;
    return diff;
  }

  // 梯形评分：落在理想区间内得 1 分，越接近硬门限得分越低，超出硬门限为 0。
  static float BandScore(float value, float ideal_lo, float ideal_hi,
                         float hard_lo, float hard_hi) {
    if (value < hard_lo || value > hard_hi) return 0.0F;
    if (value >= ideal_lo && value <= ideal_hi) return 1.0F;
    if (value < ideal_lo) {
      return (value - hard_lo) / (ideal_lo - hard_lo);
    }
    return (hard_hi - value) / (hard_hi - ideal_hi);
  }

  // 加权打分。权重之和为 1，输出落在 [0,1]，可直接当作置信度使用。
  // 理想区间取自公开数据的分布（无量纲），不依赖具体样本序号。
  static float Score(float width_ratio, float height_ratio, float angle_diff,
                     float length_ratio, float brightness_left,
                     float brightness_right) {
    const float width_score =
        BandScore(width_ratio, 0.9F, 2.3F, min_width_length_ratio,
                  max_width_length_ratio);
    const float height_score =
        BandScore(height_ratio, 0.0F, 0.12F, 0.0F, max_height_length_ratio);
    const float angle_score = BandScore(angle_diff, 0.0F, 6.0F, 0.0F,
                                        max_angle_diff_deg);
    const float length_score =
        BandScore(length_ratio, 1.0F, 1.15F, 1.0F, max_length_ratio);
    const float brightness_score =
        BandScore(std::min(brightness_left, brightness_right), 0.80F, 1.0F,
                  min_brightness, 1.0F);
    return 0.30F * width_score + 0.20F * height_score + 0.20F * angle_score +
           0.20F * length_score + 0.10F * brightness_score;
  }
};

}  // namespace rm_assessment
