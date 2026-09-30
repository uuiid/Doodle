//
// AI 灯光：rig 纯运算的实现（§15.8）
//
// 声明与 doc 在 ai_light_rig_ops.h，这里只放实现。
// 全部是纯函数：不碰数据库、不写日志（单测宿主没有 core_set::ctx_ptr 也没有 HTTP logger，
// 纯函数才能被单测直接调用 —— §15.6）。
//

#include "ai_light_rig_ops.h"

#include "ai_light_seam_common.h"

#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <string>

namespace doodle::ai_light {

// first_light_with_role 定义在同目录的 ai_light_seam_common.h（接缝共用件）：apply_axis_step
// 与接缝 2 的 map_candidate_result 共用同一份定义 —— 两处各留一份是「改了一处、忘另一处」的经典来源。

/// 当前 key:fill 比。除了 apply_axis_step，接缝 3 的提示词也要用它 —— key_fill_ratio 是
/// **绝对值**轴，模型必须知道现状才能给出新值（「对比再强一点」不是一个可直接使用的数字）。
///
/// 与 apply_axis_step 的判定条件**逐条一致**：key 存在且强度 > 0、所有 fill 灯与 key
/// **同 intensity_unit_**（坎德拉与勒克斯不是同一个量纲，比值没有意义）、fill 强度和 > 0。
std::optional<double> key_fill_ratio_of(const lighting_rig_t& in_rig) {
  // 这里不调 first_light_with_role：它取的是可写引用（apply_axis_step 要改灯），
  // 而本函数是只读的。
  const light_t* l_key = nullptr;
  for (const auto& l_light : in_rig.lights_)
    if (l_light.role_ == light_role::key) {
      l_key = &l_light;
      break;
    }
  if (l_key == nullptr) return std::nullopt;
  if (l_key->intensity_.value_ <= 0.0) return std::nullopt;
  double l_fill_sum = 0.0;
  for (const auto& l_light : in_rig.lights_) {
    if (l_light.role_ != light_role::fill) continue;
    if (l_light.intensity_.unit_ != l_key->intensity_.unit_) return std::nullopt;
    l_fill_sum += l_light.intensity_.value_;
  }
  if (l_fill_sum <= 0.0) return std::nullopt;
  return l_key->intensity_.value_ / l_fill_sum;
}

bool apply_axis_step(lighting_rig_t& io_rig, const variant_axis_t& in_axis) {
  const double l_v = in_axis.value_;
  switch (in_axis.axis_) {
    case variant_axis_kind::none:
      return false;

    case variant_axis_kind::key_direction: {
      if (l_v == 0.0) return false;
      auto* l_key = first_light_with_role(io_rig, light_role::key);
      if (l_key == nullptr) return false;
      double l_azimuth = std::fmod(l_key->offset_.azimuth_deg_ + l_v, 360.0);
      if (l_azimuth > 180.0) l_azimuth -= 360.0;
      if (l_azimuth < -180.0) l_azimuth += 360.0;
      l_key->offset_.azimuth_deg_ = l_azimuth;
      return true;
    }

    case variant_axis_kind::key_fill_ratio: {
      if (l_v <= 0.0) return false;
      // 当前比值走 key_fill_ratio_of()，与接缝 3 提示词里给模型的数字同一份定义。
      const auto l_current = key_fill_ratio_of(io_rig);
      if (!l_current.has_value()) return false;
      // 要让比值等于目标，就按 当前/目标 缩放补光。
      const double l_scale = *l_current / l_v;
      if (l_scale == 1.0) return false;
      for (auto& l_light : io_rig.lights_)
        if (l_light.role_ == light_role::fill) l_light.intensity_.value_ *= l_scale;
      return true;
    }

    case variant_axis_kind::kelvin: {
      if (l_v == 0.0) return false;
      for (auto& l_light : io_rig.lights_) {
        if (l_light.color_.mode_ != color_mode::temperature) continue;
        l_light.color_.kelvin_ = std::clamp(l_light.color_.kelvin_ + l_v, 1500.0, 15000.0);
      }
      io_rig.globals_.white_balance_.kelvin_ =
          std::clamp(io_rig.globals_.white_balance_.kelvin_ + l_v, 1500.0, 15000.0);
      // 即便所有灯都不是色温模式，全局白平衡也已经变了，所以这里恒为「改了」；
      // 真正没改的情况（已经在夹取边界上）由 map_candidate_result 的同一个候选检查兜住。
      return true;
    }

    case variant_axis_kind::hardness: {
      if (l_v == 0.0) return false;
      bool l_applied = false;
      for (auto& l_light : io_rig.lights_) {
        const double l_softness = std::clamp(l_light.shadows_.softness_ - 0.5 * l_v, 0.0, 1.0);
        const double l_radius   = std::max(0.0, l_light.shape_.source_radius_cm_ * (1.0 - 0.5 * l_v));
        if (l_softness != l_light.shadows_.softness_ || l_radius != l_light.shape_.source_radius_cm_)
          l_applied = true;
        l_light.shadows_.softness_       = l_softness;
        l_light.shape_.source_radius_cm_ = l_radius;
      }
      return l_applied;
    }

    case variant_axis_kind::ambient_level: {
      if (l_v == 0.0) return false;
      const double l_scale = std::clamp(1.0 + l_v, 0.0, 4.0);
      io_rig.globals_.environment_.sky_light_intensity_ =
          std::max(0.0, io_rig.globals_.environment_.sky_light_intensity_ * l_scale);
      for (auto& l_light : io_rig.lights_)
        if (l_light.role_ == light_role::ambient)
          l_light.intensity_.value_ = std::max(0.0, l_light.intensity_.value_ * l_scale);
      return true;
    }

    case variant_axis_kind::grading_strength: {
      if (l_v == 0.0) return false;
      // 只放大/缩小「偏离中性的量」：contrast/saturation 的中性值是 1，其余是 0。
      // 不碰 lights_ —— 这个轴是纯调色的自由度，动灯会让「调色 vs 布光」两个方向混在一起。
      const double l_scale = std::clamp(1.0 + l_v, 0.0, 3.0);
      auto& l_grading      = io_rig.globals_.grading_;
      l_grading.contrast_  = std::clamp(1.0 + (l_grading.contrast_ - 1.0) * l_scale, 0.0, 3.0);
      l_grading.saturation_ = std::clamp(1.0 + (l_grading.saturation_ - 1.0) * l_scale, 0.0, 3.0);
      l_grading.lift_       = std::clamp(l_grading.lift_ * l_scale, 0.0, 1.0);
      l_grading.vignette_   = std::clamp(l_grading.vignette_ * l_scale, 0.0, 1.0);
      l_grading.bloom_      = std::clamp(l_grading.bloom_ * l_scale, 0.0, 1.0);
      return true;
    }

    default:
      return false;
  }
}

std::vector<lighting_rig_t> expand_candidates(const lighting_rig_t& in_base, const iteration_axes_t& in_axes) {
  std::vector<lighting_rig_t> l_ret{};
  l_ret.reserve(1 + g_axis_step_budget);

  // 候选 0 是本轮基线：6 个候选是「1 个基线 + 5 个变异」，不是 6 个平行实验。
  // 有基线在场，美术才看得出「这次改动做了什么」，ops_ 也才有参照物（§15.8）。
  lighting_rig_t l_zero       = in_base;
  l_zero.variant_axis_        = variant_axis_t{};
  l_zero.variant_axis_.axis_  = variant_axis_kind::none;
  l_zero.variant_axis_.value_ = 0.0;
  l_zero.variant_axis_.label_ = "基线";
  l_zero.ops_.clear();
  l_ret.emplace_back(std::move(l_zero));

  for (const auto& l_axis : in_axes.axes_) {
    for (const double l_step : l_axis.steps_) {
      lighting_rig_t l_rig = in_base;
      variant_axis_t l_variant{};
      l_variant.axis_  = l_axis.axis_;
      l_variant.value_ = l_step;
      l_variant.label_ = l_axis.label_;
      l_rig.variant_axis_ = l_variant;
      apply_axis_step(l_rig, l_variant);
      l_rig.ops_.clear();
      l_ret.emplace_back(std::move(l_rig));
    }
  }
  return l_ret;
}

std::vector<light_op_t> diff_rigs(const lighting_rig_t& in_base, const lighting_rig_t& in_target) {
  std::vector<light_op_t> l_ops{};
  const std::string l_rationale =
      in_target.variant_axis_.axis_ == variant_axis_kind::none
          ? std::string{"基线"}
          : fmt::format("{} {}", magic_enum::enum_name(in_target.variant_axis_.axis_), in_target.variant_axis_.value_);

  // ---- 全局量：四块各对应一个 light_op_kind，只发真的变了的那几块 ----
  // patch 用「目标值整块」而不是增量：语义是 set，与 UE 侧「按 op 精确设置」一致，
  // 也与 §7.5 的「按 op 精确增删、不用 ClearAllLight」一致。
  if (nlohmann::json{in_base.globals_} != nlohmann::json{in_target.globals_}) {
    const std::pair<light_op_kind, const char*> l_blocks[] = {
        {light_op_kind::set_exposure, "exposure_"},
        {light_op_kind::set_white_balance, "white_balance_"},
        {light_op_kind::set_environment, "environment_"},
        {light_op_kind::set_grading, "grading_"},
    };
    const auto l_base_json = nlohmann::json{in_base.globals_};
    const auto l_tgt_json  = nlohmann::json{in_target.globals_};
    for (const auto& [l_kind, l_key] : l_blocks) {
      if (!l_base_json.contains(l_key) || l_base_json.at(l_key) == l_tgt_json.at(l_key)) continue;
      light_op_t l_op{};
      l_op.op_        = l_kind;
      l_op.patch_     = l_tgt_json.at(l_key);
      l_op.rationale_ = l_rationale;
      l_ops.emplace_back(std::move(l_op));
    }
  }

  // ---- 逐灯：以 id_ 对齐。展开器只改变量、不增删灯，所以两边 id 集合必然相同，
  //      这里也不会产生 add / remove（§6.6 的护栏正是这个意思）----
  const auto l_count = std::min(in_base.lights_.size(), in_target.lights_.size());
  for (std::size_t l_i = 0; l_i < l_count; ++l_i) {
    const auto& l_base_light = in_base.lights_.at(l_i);
    const auto& l_tgt_light  = in_target.lights_.at(l_i);
    if (l_base_light.id_ != l_tgt_light.id_) continue;
    // 逐子块比较（用类型化成员比较，patch 的键名是给执行器看的稳定名字）——
    // 这样 patch 里只有真的变了的子块，审计和执行器都看得清「这次动了什么」。
    nlohmann::json l_patch = nlohmann::json::object();
    if (nlohmann::json{l_base_light.offset_} != nlohmann::json{l_tgt_light.offset_})
      l_patch["offset"] = l_tgt_light.offset_;
    if (nlohmann::json{l_base_light.intensity_} != nlohmann::json{l_tgt_light.intensity_})
      l_patch["intensity"] = l_tgt_light.intensity_;
    if (nlohmann::json{l_base_light.color_} != nlohmann::json{l_tgt_light.color_})
      l_patch["color"] = l_tgt_light.color_;
    if (nlohmann::json{l_base_light.shape_} != nlohmann::json{l_tgt_light.shape_})
      l_patch["shape"] = l_tgt_light.shape_;
    if (nlohmann::json{l_base_light.shadows_} != nlohmann::json{l_tgt_light.shadows_})
      l_patch["shadows"] = l_tgt_light.shadows_;
    if (l_patch.empty()) continue;
    light_op_t l_op{};
    l_op.op_        = light_op_kind::modify;
    l_op.light_     = l_tgt_light.id_;
    l_op.patch_     = std::move(l_patch);
    l_op.rationale_ = l_rationale;
    l_ops.emplace_back(std::move(l_op));
  }
  return l_ops;
}

}  // namespace doodle::ai_light
