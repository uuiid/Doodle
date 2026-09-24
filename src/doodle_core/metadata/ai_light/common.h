//
// AI 灯光：共享数值原语与 schema 版本常量
//
// 从 rig.h / spec.h / candidate.h 都要用到 interval_t，放在这里避免它们互相包含。
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <string_view>

namespace doodle::ai_light {

/// LightingRig IR 版本（docs/ue_ai_lighting_plan.md §4.3）
constexpr std::string_view g_rig_schema_version{"1.1"};
/// LightingSpec 版本（docs/ue_ai_lighting_plan.md §4.2）
/// 1.2 → 1.3：Tier B 取消后 tier 与 degrade_level 信息等价，删掉 tier，只留 degrade_level。
constexpr std::string_view g_spec_schema_version{"1.3"};

/// 闭区间 [low, high]。Spec 的目标全部是区间而不是点值 —— 这是「参考是近似而非精确」
/// 这一前提在数据契约上的体现，也是评判层「落点是否在区间内」的判据。
/// JSON 里是定长数组 [low, high]。
struct DOODLE_CORE_API interval_t {
  double low_{0.0};
  double high_{0.0};

  bool contains(double in_value) const { return in_value >= low_ && in_value <= high_; }
  double center() const { return (low_ + high_) * 0.5; }
  double width() const { return high_ - low_; }

  friend void to_json(nlohmann::json& j, const interval_t& p) { j = nlohmann::json::array({p.low_, p.high_}); }
  friend void from_json(const nlohmann::json& j, interval_t& p) {
    if (!j.is_array() || j.size() != 2)
      throw nlohmann::json::type_error::create(302, "interval_t 需要长度为 2 的数组", &j);
    j.at(0).get_to(p.low_);
    j.at(1).get_to(p.high_);
  }
};

/// 嵌套结构落库的取用助手。
///
/// 为什么需要它：本库的 ORM 只为有限的一组类型提供了列绑定
/// （标量 / std::string / uuid / time_point / zoned_time / FSys::path / 枚举 /
/// std::vector<std::string> / std::vector<枚举> / nlohmann::json / std::optional<T>，
/// 见 sqlite_orm/orm/sqlite_statement.h）。**嵌套结构体与其它容器都不是列类型**。
/// 所以凡是嵌套结构（intent / targets / rig / metrics / judgement …）一律以
/// nlohmann::json 成员落库，再靠各自的 to_json / from_json 做类型化访问。
template <typename T>
T json_get_or_default(const nlohmann::json& in_json) {
  if (in_json.is_null()) return T{};
  return in_json.get<T>();
}

}  // namespace doodle::ai_light
