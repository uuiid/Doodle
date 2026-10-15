//
// AI 灯光：共享数值原语
//
// 从 rig.h / spec.h / candidate.h 都要用到 interval_t，放在这里避免它们互相包含。
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle::ai_light {

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

// ---------------------------------------------------------------------------
// std::optional ↔ JSON：**用库自带的，不要自己写**
// ---------------------------------------------------------------------------
//
// nlohmann 从 C++17 起自带 std::optional 的序列化，本工程用的版本有，
// 且本工程编译时 JSON_HAS_CPP_17 成立，所以它一定生效：
//
//   读：nlohmann/detail/conversions/from_json.hpp:52-63
//       null      → std::nullopt
//       其它      → opt.emplace(j.get<T>())
//   写：nlohmann/detail/conversions/to_json.hpp:271-281
//       有值      → j = *opt（递归走 T 自己的 to_json / adl_serializer）
//       无值      → j = nullptr
//
// 所以直接赋值就行，不要写 `opt ? X : Y` 这种三元转换，也不要包一层助手函数：
//
//     j["judgement_due_at"] = p.judgement_due_at_;          // 对
//     if (j.contains("x")) j.at("x").get_to(p.x_);          // 对（null 自动变 nullopt）
//
// **唯一的坑是别用花括号**：`nlohmann::json{*p.at_}` 是列表初始化，
// 会优先匹配 basic_json(initializer_list_t) 构造函数，结果是**单元素数组** `[x]`
// 而不是 `x`。这个坑极其隐蔽：代码合法、编译通过、写库成功，只有读回来时才炸 ——
// from_json 里 get_ref<const std::string&>() 对着数组抛 type_error.303，
// 而报错信息完全看不出是哪个字段写坏的。同一个坑对 std::string / double /
// 结构体一样成立（`nlohmann::json{some_string}` 也是数组，rig.h 的 ies_ 就这么丢过）。
// 要么直接赋值，要么用括号 `nlohmann::json(x)`（括号走普通重载决议）。

}  // namespace doodle::ai_light
