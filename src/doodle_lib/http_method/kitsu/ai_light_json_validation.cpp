//
// AI 灯光：模型输出 JSON 架构（schema）与读取/校验的唯一一层 —— 实现
// （设计见 docs/ue_ai_lighting_plan.md §15.11）
//
// ---------------------------------------------------------------------------
// 两条接缝的验证策略表（**刻意不同**，共用的是 schema 建造器，不是策略）
// ---------------------------------------------------------------------------
// | 接缝                | 策略 | 读不出字段时   | 顶层非 object 时 | 语义无效时         |
// | ------------------- | ---- | -------------- | ---------------- | ------------------ |
// | generate_candidates | 严格 | 抛 502         | 抛 502           | 抛 502（颜色分支） |
// | structure_feedback  | 混合 | 形状违规抛 502 | 抛 502           | 静默丢弃（不落库） |
//
// 曾经还有第一行 generate_spec（宽松：读不出字段保留统计推导值、语义无效保留默认值）。
// 它随 spec 接缝一起删除 —— 蒸馏方案归前端后，那条「宽松」策略在服务端没有消费者了。
//
// 这张表是重构的**验收条件之一**（方案成功标准 4）：两套策略必须原样保留，由既有测试的
// 断言守住 —— ai_light_candidates（抛错路径）/ ai_light_feedback（丢弃路径）。
//
// 本文件是 `additionalProperties` 字面量与 require_*/opt_* 家族的**唯一**定义处。
//
#include "ai_light_json_validation.h"

// 校验库只出现在这里：公共头不暴露它（见 .h 顶部约定）。
#include <nlohmann/json-schema.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <exception>

namespace doodle::ai_light::json_val {

namespace {

/// 点分路径的一段：`a` → {key:"a"}；`a[0]` → {key:"a", index:0}；`[0]` → {index:0}。
struct path_segment_t {
  std::string key_{};
  std::optional<std::size_t> index_{};
};

/// 解析 `a.b[0].c` 形状的路径。空段（连续的点、末尾的点）按「不存在」处理 ——
/// 与其猜调用方的意思，不如让它查不到并在消息里暴露出来。
std::vector<path_segment_t> split_path(std::string_view in_path) {
  std::vector<path_segment_t> l_ret{};
  path_segment_t l_cur{};
  std::size_t l_i = 0;
  while (l_i < in_path.size()) {
    const char l_c = in_path[l_i];
    if (l_c == '.') {
      if (l_cur.key_.empty() && !l_cur.index_.has_value()) return {};  // 空段
      l_ret.emplace_back(std::move(l_cur));
      l_cur = path_segment_t{};
      ++l_i;
    } else if (l_c == '[') {
      const auto l_close = in_path.find(']', l_i);
      if (l_close == std::string_view::npos) return {};
      const auto l_digits = in_path.substr(l_i + 1, l_close - l_i - 1);
      if (l_digits.empty() || l_digits.find_first_not_of("0123456789") != std::string_view::npos) return {};
      l_cur.index_ = static_cast<std::size_t>(std::stoull(std::string{l_digits}));
      l_i          = l_close + 1;
      // `a[0][1]`：下标之后紧跟下标时当前段已经完整，先收起来（`.` 的情况交给下一轮的
      // `.` 分支处理，那里同样会收段）。
      if (l_i < in_path.size() && in_path[l_i] == '[') {
        l_ret.emplace_back(std::move(l_cur));
        l_cur = path_segment_t{};
      }
    } else {
      l_cur.key_ += l_c;
      ++l_i;
    }
  }
  // 尾随的点 = 空段 → 路径不合法（json_at_path 会返回 nullptr，让错位在消息里暴露出来）
  if (l_cur.key_.empty() && !l_cur.index_.has_value()) return {};
  l_ret.emplace_back(std::move(l_cur));
  return l_ret;
}

/// schema 一致性走查：递归，路径是 `$.a.b[0]` 形状。
void walk_schema_consistency(const nlohmann::json& in_node, const std::string& in_path, std::vector<std::string>& io_out) {
  if (!in_node.is_object()) {
    io_out.push_back(fmt::format("{}：schema 节点不是 object", in_path));
    return;
  }
  if (in_node.contains("properties")) {
    const auto& l_props = in_node.at("properties");
    if (!l_props.is_object()) {
      io_out.push_back(fmt::format("{}.properties：不是 object", in_path));
    } else {
      // ① 必须 additionalProperties:false（否则模型多回字段会被静默接受）
      if (!in_node.contains("additionalProperties") || in_node.at("additionalProperties") != nlohmann::json(false)) {
        io_out.push_back(fmt::format("{}：object 缺 additionalProperties:false", in_path));
      }
      // ② required 必须恰好等于属性名集合
      std::vector<std::string> l_names{};
      for (auto&& [l_key, l_value] : l_props.items()) l_names.push_back(l_key);
      std::vector<std::string> l_required{};
      if (in_node.contains("required") && in_node.at("required").is_array()) {
        for (auto&& l_v : in_node.at("required"))
          if (l_v.is_string()) l_required.push_back(l_v.get<std::string>());
      } else {
        io_out.push_back(fmt::format("{}.required：缺失或不是数组", in_path));
      }
      auto l_sorted_names    = l_names;
      auto l_sorted_required = l_required;
      std::sort(l_sorted_names.begin(), l_sorted_names.end());
      std::sort(l_sorted_required.begin(), l_sorted_required.end());
      if (l_sorted_names != l_sorted_required) {
        io_out.push_back(fmt::format(
            "{}：required 与属性名集合不一致（属性 {} 个，required {} 个）", in_path, l_names.size(), l_required.size()
        ));
      }
      for (auto&& [l_key, l_value] : l_props.items()) walk_schema_consistency(l_value, in_path + "." + l_key, io_out);
    }
  }
  if (in_node.contains("type") && in_node.at("type").is_string() && in_node.at("type").get<std::string>() == "array") {
    // ③ array 必须有 items，且 minItems <= maxItems
    if (!in_node.contains("items")) {
      io_out.push_back(fmt::format("{}：array 缺 items", in_path));
    } else {
      walk_schema_consistency(in_node.at("items"), in_path + "[]", io_out);
    }
    if (in_node.contains("minItems") && in_node.contains("maxItems") && in_node.at("minItems").is_number() &&
        in_node.at("maxItems").is_number() && in_node.at("minItems").get<std::size_t>() > in_node.at("maxItems").get<std::size_t>()) {
      io_out.push_back(fmt::format("{}：minItems > maxItems", in_path));
    }
  }
}

/// 取值 + 类型检查：取不到就抛，消息统一带接缝名与字段路径。
const nlohmann::json& require_node(
    const nlohmann::json& in_root, std::string_view in_seam, std::string_view in_path, std::string_view in_expected
) {
  const auto* l_node = json_at_path(in_root, in_path);
  DOODLE_CHICK_HTTP(
      l_node != nullptr, bad_gateway, "{}：字段 {} 缺失或类型不符合（期望{}）", in_seam, in_path, in_expected
  );
  return *l_node;
}

}  // namespace

// ===========================================================================
// 路径解析
// ===========================================================================

const nlohmann::json* json_at_path(const nlohmann::json& in_root, std::string_view in_path) {
  const auto l_segments = split_path(in_path);
  if (l_segments.empty() && !in_path.empty()) return nullptr;
  const nlohmann::json* l_cur = &in_root;
  for (const auto& l_seg : l_segments) {
    if (l_cur == nullptr) return nullptr;
    if (!l_seg.key_.empty()) {
      if (!l_cur->is_object()) return nullptr;
      const auto l_it = l_cur->find(l_seg.key_);
      if (l_it == l_cur->end()) return nullptr;
      l_cur = &(*l_it);
    }
    if (l_seg.index_.has_value()) {
      if (!l_cur->is_array() || l_cur->size() <= *l_seg.index_) return nullptr;
      l_cur = &l_cur->at(*l_seg.index_);
    }
  }
  return l_cur;
}

// ===========================================================================
// 3.1 建造器
// ===========================================================================

nlohmann::json schema_object(nlohmann::json in_props) {
  std::vector<std::string> l_required{};
  for (auto&& [l_key, l_value] : in_props.items()) l_required.push_back(l_key);
  return schema_object(std::move(in_props), std::move(l_required));
}

nlohmann::json schema_object(nlohmann::json in_props, std::vector<std::string> in_required) {
  return nlohmann::json{
      {"type", "object"},
      {"properties", std::move(in_props)},
      {"required", std::move(in_required)},
      {"additionalProperties", false}
  };
}

nlohmann::json schema_array(nlohmann::json in_items, std::optional<std::size_t> in_min, std::optional<std::size_t> in_max) {
  nlohmann::json l_ret{{"type", "array"}, {"items", std::move(in_items)}};
  if (in_min.has_value()) l_ret["minItems"] = *in_min;
  if (in_max.has_value()) l_ret["maxItems"] = *in_max;
  return l_ret;
}

nlohmann::json schema_number(std::optional<double> in_min, std::optional<double> in_max) {
  nlohmann::json l_ret{{"type", "number"}};
  if (in_min.has_value()) l_ret["minimum"] = *in_min;
  if (in_max.has_value()) l_ret["maximum"] = *in_max;
  return l_ret;
}

nlohmann::json schema_string() { return nlohmann::json{{"type", "string"}}; }

nlohmann::json schema_bool() { return nlohmann::json{{"type", "boolean"}}; }

nlohmann::json schema_string_enum(std::span<const std::string_view> in_values) {
  nlohmann::json l_enum = nlohmann::json::array();
  for (auto&& l_v : in_values) l_enum.emplace_back(l_v);
  return nlohmann::json{{"type", "string"}, {"enum", l_enum}};
}

nlohmann::json schema_fixed_number_array(std::size_t in_size) {
  return schema_array(schema_number(), in_size, in_size);
}

nlohmann::json schema_string_array() { return schema_array(schema_string()); }

// ===========================================================================
// 3.2 严格读取器
// ===========================================================================

nlohmann::json require_object(const nlohmann::json& in_root, std::string_view in_seam, std::string_view in_path) {
  const auto& l_node = require_node(in_root, in_seam, in_path, "object");
  DOODLE_CHICK_HTTP(l_node.is_object(), bad_gateway, "{}：字段 {} 缺失或类型不符合（期望object）", in_seam, in_path);
  return l_node;
}

nlohmann::json require_array(const nlohmann::json& in_root, std::string_view in_seam, std::string_view in_path) {
  const auto& l_node = require_node(in_root, in_seam, in_path, "array");
  DOODLE_CHICK_HTTP(l_node.is_array(), bad_gateway, "{}：字段 {} 缺失或类型不符合（期望array）", in_seam, in_path);
  return l_node;
}

double require_number(const nlohmann::json& in_root, std::string_view in_seam, std::string_view in_path) {
  const auto& l_node = require_node(in_root, in_seam, in_path, "数字");
  DOODLE_CHICK_HTTP(l_node.is_number(), bad_gateway, "{}：字段 {} 缺失或类型不符合（期望数字）", in_seam, in_path);
  return l_node.get<double>();
}

std::string require_string(const nlohmann::json& in_root, std::string_view in_seam, std::string_view in_path) {
  const auto& l_node = require_node(in_root, in_seam, in_path, "字符串");
  DOODLE_CHICK_HTTP(l_node.is_string(), bad_gateway, "{}：字段 {} 缺失或类型不符合（期望字符串）", in_seam, in_path);
  return l_node.get<std::string>();
}

bool require_bool(const nlohmann::json& in_root, std::string_view in_seam, std::string_view in_path) {
  const auto& l_node = require_node(in_root, in_seam, in_path, "布尔值");
  DOODLE_CHICK_HTTP(l_node.is_boolean(), bad_gateway, "{}：字段 {} 缺失或类型不符合（期望布尔值）", in_seam, in_path);
  return l_node.get<bool>();
}

std::array<double, 3> require_vec3(const nlohmann::json& in_root, std::string_view in_seam, std::string_view in_path) {
  const auto& l_node = require_node(in_root, in_seam, in_path, "3 个数字的数组");
  const bool l_ok     = l_node.is_array() && l_node.size() == 3 && l_node.at(0).is_number() && l_node.at(1).is_number() &&
                    l_node.at(2).is_number();
  DOODLE_CHICK_HTTP(l_ok, bad_gateway, "{}：字段 {} 缺失或类型不符合（期望 3 个数字的数组）", in_seam, in_path);
  return std::array<double, 3>{l_node.at(0).get<double>(), l_node.at(1).get<double>(), l_node.at(2).get<double>()};
}

std::string require_one_of(
    const nlohmann::json& in_root, std::string_view in_seam, std::string_view in_path,
    std::span<const std::string_view> in_allowed
) {
  const auto l_value = require_string(in_root, in_seam, in_path);
  const bool l_ok    = std::find(in_allowed.begin(), in_allowed.end(), std::string_view{l_value}) != in_allowed.end();
  DOODLE_CHICK_HTTP(l_ok, bad_gateway, "{}：字段 {} 的值 \"{}\" 不在允许集合内", in_seam, in_path, l_value);
  return l_value;
}

// ===========================================================================
// 3.3 宽松读取器
// ===========================================================================

std::optional<nlohmann::json> opt_object(const nlohmann::json& in_parent, std::string_view in_key) {
  if (!in_parent.is_object()) return std::nullopt;
  const auto l_it = in_parent.find(std::string{in_key});
  if (l_it == in_parent.end() || !l_it->is_object()) return std::nullopt;
  // 必须写成 in_place 的显式构造：本工程里 `const nlohmann::json&` **不能隐式**转成
  // `std::optional<nlohmann::json>`（`return *l_it;` 报 C2440「No user-defined-conversion operator
  // available」）。探针 `E:\Doodle\build\probe_optional.cpp` 实测：隐式 `return in_json;` 编不过，
  // 而 `{std::in_place, in_json}` / `emplace(...)` / `std::optional<json>(in_json)` 三种显式写法都行
  // —— 说明 nlohmann::json 的复制构造在当前配置下让 `is_convertible<const json&, json>` 为假，
  // 于是 std::optional 的转换构造被实例化成 explicit。这里选 in_place，语义最直白。
  return std::optional<nlohmann::json>{std::in_place, *l_it};
}

std::optional<std::string> opt_string(const nlohmann::json& in_parent, std::string_view in_key) {
  if (!in_parent.is_object()) return std::nullopt;
  const auto l_it = in_parent.find(std::string{in_key});
  if (l_it == in_parent.end() || !l_it->is_string()) return std::nullopt;
  auto l_v = l_it->get<std::string>();
  if (l_v.empty()) return std::nullopt;
  return l_v;
}

std::optional<bool> opt_bool(const nlohmann::json& in_parent, std::string_view in_key) {
  if (!in_parent.is_object()) return std::nullopt;
  const auto l_it = in_parent.find(std::string{in_key});
  if (l_it == in_parent.end() || !l_it->is_boolean()) return std::nullopt;
  return l_it->get<bool>();
}

std::optional<double> opt_number(const nlohmann::json& in_parent, std::string_view in_key) {
  if (!in_parent.is_object()) return std::nullopt;
  const auto l_it = in_parent.find(std::string{in_key});
  if (l_it == in_parent.end() || !l_it->is_number()) return std::nullopt;
  return l_it->get<double>();
}

std::optional<interval_t> opt_interval(const nlohmann::json& in_parent, std::string_view in_key) {
  if (!in_parent.is_object()) return std::nullopt;
  const auto l_it = in_parent.find(std::string{in_key});
  if (l_it == in_parent.end() || !l_it->is_array() || l_it->size() != 2 || !l_it->at(0).is_number() ||
      !l_it->at(1).is_number())
    return std::nullopt;
  // 注意：这里的 low/high 必须先取出来再构造 interval_t，**不能**用 `interval_t l_i{*l_it}`
  // —— nlohmann 的 initializer_list 会把「一个 json 数组」变成「一个元素的 interval_t」。
  interval_t l_ret{};
  l_ret.low_  = l_it->at(0).get<double>();
  l_ret.high_ = l_it->at(1).get<double>();
  if (l_ret.low_ > l_ret.high_) {
    // 模型把区间写反（[high, low]）时静默交换：让它直接生效比让下游「落点永远不在区间内」
    // 好排查一个数量级。
    std::swap(l_ret.low_, l_ret.high_);
  }
  return l_ret;
}

std::vector<std::string> opt_string_array(const nlohmann::json& in_parent, std::string_view in_key) {
  if (!in_parent.is_object()) return {};
  const auto l_it = in_parent.find(std::string{in_key});
  if (l_it == in_parent.end() || !l_it->is_array()) return {};
  std::vector<std::string> l_ret{};
  for (auto&& l_v : *l_it) {
    if (!l_v.is_string()) return {};  // 有一个不是字符串就整体当「没给」
    l_ret.push_back(l_v.get<std::string>());
  }
  return l_ret;
}

std::optional<std::string> opt_one_of(
    const nlohmann::json& in_parent, std::string_view in_key, std::span<const std::string_view> in_allowed
) {
  auto l_value = opt_string(in_parent, in_key);
  if (!l_value.has_value()) return std::nullopt;
  if (std::find(in_allowed.begin(), in_allowed.end(), std::string_view{*l_value}) == in_allowed.end()) return std::nullopt;
  return l_value;
}

// ===========================================================================
// 3.4 架构校验（诊断用，不作准入闸门 —— 见 .h 顶部 D-1）
// ===========================================================================

std::vector<std::string> check_schema_consistency(const nlohmann::json& in_schema) {
  std::vector<std::string> l_out{};
  if (!in_schema.is_object()) {
    l_out.push_back("$：schema 顶层不是 object");
    return l_out;
  }
  walk_schema_consistency(in_schema, "$", l_out);
  return l_out;
}

std::vector<std::string> validate_schema_is_wellformed(const nlohmann::json& in_schema, std::string_view in_seam) {
  try {
    // 构造时就编译 schema；schema 不合法会在这里抛。**这是我们自己的 bug，不是模型的错。**
    nlohmann::json_schema::json_validator l_validator{in_schema};
    (void)l_validator;
  } catch (const std::exception& l_e) {
    return {fmt::format("{}：schema 本身不是合法 draft-7（{}）", in_seam, l_e.what())};
  }
  return {};
}

namespace {

/// 一次收齐全部违规（而不是读到第一个坏字段就停）：basic_error_handler 只记「有没有错」，
/// 诊断需要每一条的 json pointer。
class collecting_error_handler_t : public nlohmann::json_schema::error_handler {
 public:
  explicit collecting_error_handler_t(std::vector<std::string>& io_out) : out_(io_out) {}

  void error(const nlohmann::json::json_pointer& in_ptr, const nlohmann::json& in_instance, const std::string& in_message)
      override {
    out_.push_back(fmt::format("{}: {}", in_ptr.to_string(), in_message));
  }

 private:
  std::vector<std::string>& out_;
};

}  // namespace

std::vector<std::string> validate_model_output(
    const nlohmann::json& in_value, const nlohmann::json& in_schema, std::string_view in_seam
) {
  std::vector<std::string> l_out{};
  try {
    nlohmann::json_schema::json_validator l_validator{in_schema};
    collecting_error_handler_t l_handler{l_out};
    l_validator.validate(in_value, l_handler);
  } catch (const std::exception& l_e) {
    l_out.push_back(fmt::format("{}：校验过程本身抛错（{}）", in_seam, l_e.what()));
  }
  return l_out;
}

std::string join_violations(const std::vector<std::string>& in_violations, std::size_t in_max) {
  if (in_violations.empty()) return "无";
  const auto l_n = std::min(in_max, in_violations.size());
  std::string l_ret{};
  for (std::size_t l_i = 0; l_i < l_n; ++l_i) {
    if (l_i != 0) l_ret += "；";
    l_ret += in_violations[l_i];
  }
  if (in_violations.size() > l_n) l_ret += fmt::format("（另有 {} 条已省略）", in_violations.size() - l_n);
  return l_ret;
}

}  // namespace doodle::ai_light::json_val
