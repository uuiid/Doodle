//
// AI 灯光：rig 纯运算的实现
//
// 声明与 doc 在 ai_light_rig_ops.h，这里只放实现。
// 全部是纯函数：不碰数据库、不写日志（单测宿主没有 core_set::ctx_ptr 也没有 HTTP logger，
// 纯函数才能被单测直接调用 —— §15.6）。
//

#include "ai_light_rig_ops.h"

#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <string>

namespace doodle::ai_light {

/// 当前 key:fill 比。接缝 3 的提示词要用它 —— key_fill_ratio 是**绝对值**轴，
/// 模型必须知道现状才能给出新值（「对比再强一点」不是一个可直接使用的数字）。
std::optional<double> key_fill_ratio_of(const lighting_rig_t& in_rig) {
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

std::vector<light_op_t> diff_rigs(const lighting_rig_t& in_base, const lighting_rig_t& in_target) {
  std::vector<light_op_t> l_ops{};
  const std::string l_rationale =
      in_target.variant_axis_.axis_ == variant_axis_kind::none
          ? std::string{"基线"}
          : fmt::format("{} {}", magic_enum::enum_name(in_target.variant_axis_.axis_), in_target.variant_axis_.value_);

  // ---- 全局量：四块各对应一个 light_op_kind，只发真的变了的那几块 ----
  // patch 用「目标值整块」而不是增量：语义是 set，与 UE 侧「按 op 精确设置」一致，
  // 也与 §7.5 的「按 op 精确增删、不用 ClearAllLight」一致。
  // 注意用**拷贝初始化**而不是 nlohmann::json{x}：花括号走 initializer_list 构造，得到的是
  // 单元素数组，下面的 contains / at(...) 会全部落空 —— 结果就是「全局量变了却一个 op 都不发」。
  // 键名必须与 lighting_globals_t 的 to_json 完全一致（rig.h:508-511，**没有**尾下划线）。
  const nlohmann::json l_base_json = in_base.globals_;
  const nlohmann::json l_tgt_json  = in_target.globals_;
  if (l_base_json != l_tgt_json) {
    const std::pair<light_op_kind, const char*> l_blocks[] = {
        {light_op_kind::set_exposure, "exposure"},
        {light_op_kind::set_white_balance, "white_balance"},
        {light_op_kind::set_environment, "environment"},
        {light_op_kind::set_grading, "grading"},
    };
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
    // 用**圆括号**构造 json（花括号会得到单元素数组，虽然这里两侧同形后比较结果不变，
    // 但下一个改这里的人会以为它是对象 —— §15.16 坑 57）。
    nlohmann::json l_patch = nlohmann::json::object();
    if (nlohmann::json(l_base_light.offset_) != nlohmann::json(l_tgt_light.offset_))
      l_patch["offset"] = l_tgt_light.offset_;
    if (nlohmann::json(l_base_light.intensity_) != nlohmann::json(l_tgt_light.intensity_))
      l_patch["intensity"] = l_tgt_light.intensity_;
    if (nlohmann::json(l_base_light.color_) != nlohmann::json(l_tgt_light.color_))
      l_patch["color"] = l_tgt_light.color_;
    if (nlohmann::json(l_base_light.shape_) != nlohmann::json(l_tgt_light.shape_))
      l_patch["shape"] = l_tgt_light.shape_;
    if (nlohmann::json(l_base_light.shadows_) != nlohmann::json(l_tgt_light.shadows_))
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
