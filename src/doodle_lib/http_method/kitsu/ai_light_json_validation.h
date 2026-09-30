//
// AI 灯光：模型输出 JSON 架构（schema）与读取/校验的**唯一一层**
// （docs/ue_ai_lighting_plan.md §15.11；重构方案 §3）
//
// 这一层存在的理由：三条接缝此前各写了一套「模型输出怎么读」，其中被复用的那套把接缝名
// 硬编码在错误消息里（「候选基线 rig 的字段 …」），另两条接缝一复用就会报错措辞错人。
// 现在 schema 形状与取值器只有一处定义，接缝名与字段路径由调用方传进来。
//
// 本层提供三件事：
//   1. 建造 schema（§3.1）：三个接缝的 json_schema 都由这里的建造器拼出来。
//      **required 默认等于全部 properties** —— 三个接缝现有的 16 个 object 本来就是
//      「满配 additionalProperties:false + required 覆盖全部属性」，所以自动生成 required
//      与现状等价（方案 §3.1 已逐条核对）。需要「部分必填」时用显式 required 的重载。
//   2. 读取 schema（§3.2 严格 / §3.3 宽松）：
//      * 严格读取器抛 doodle_error{bad_gateway}，消息形如
//        「<接缝>：字段 <路径> 缺失或类型不符合（期望数字）」；
//      * 宽松读取器返回 optional / 空容器，**永不抛**。
//   3. 架构校验（§3.4）：我们自己的三条约定自检 + 用 pboettch/json-schema-validator 校验
//      「schema 本身是不是合法 draft-7」与「模型输出违反了我们 schema 的哪些地方」。
//      **决策 D-1：校验结果只作诊断（写日志），不作准入闸门。** 三条理由：
//        * 方舟侧 `json_schema_strict = true` 已经保证合规输出；
//        * spec 接缝**有意**容忍截断/不完整的输出（靠统计默认值 + evidence.source =
//          default_value 留痕），一道 schema 闸门会直接破坏这个设计；
//        * `additionalProperties:false` 若当闸门，会把今天被静默忽略的「模型多回一个无关键」
//          升级成整轮 502。
//      升级路径：真机日志证明模型从不违规后，把某个接缝的违规提升为 502 是**一行改动**
//      （validate_model_output() 非空时抛），届时另开一次决策。
//
// 不变约定（改动前先读这三条）：
//   * 模板（require_enum<E> / schema_enum<E>）**必须 inline 在本头里** —— 模板无法导出；
//     非模板函数定义在 .cpp 里并带 DOODLELIB_API（放匿名命名空间 = 内部链接 = LNK2019，
//     本仓库既有教训）。
//   * 本头**不得 include <nlohmann/json-schema.hpp>**：校验库只出现在 .cpp 里，
//     我们的公共 API 继续只暴露 nlohmann::json。
//   * 枚举一律走 magic_enum::enum_cast 显式校验。**绝不能**用
//     NLOHMANN_JSON_SERIALIZE_ENUM 的裸 get<>：它遇到认不出的字符串不抛，而是静默落到
//     第一个枚举值（nlohmann/detail/macro_scope.hpp:247）—— 那等于把「模型输出了一个非法
//     灯角色」变成一盏看起来正常的 key 灯，整轮候选都建立在一个错值上。
//
// 三条接缝的**验证策略刻意不同**（共用的是 schema 建造器，不是策略）：
//   spec 宽松（读不出用默认值）/ candidates 严格（抛 502）/ feedback 混合（形状抛、语义丢）。
//   见 .cpp 顶部的策略表。
//
#pragma once

#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_light/common.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace doodle::ai_light::json_val {

// ===========================================================================
// 路径解析
// ===========================================================================

/// 按点分路径取值，支持 `a`、`a.b`、`a[0]`、`a[0].b`。取不到返回 nullptr（不抛）。
/// 存在的意义是让**错误消息里的字段路径**与**实际读的那个值**是同一个字符串 ——
/// 两者分开传时最容易出现「消息说 softness、代码读的是 softness_」这种错位。
DOODLELIB_API const nlohmann::json* json_at_path(const nlohmann::json& in_root, std::string_view in_path);

// ===========================================================================
// 3.1 架构建造器（唯一一处定义 schema 形状）
// ===========================================================================

/// `{type:object, properties, required(= 全部属性名), additionalProperties:false}`。
DOODLELIB_API nlohmann::json schema_object(nlohmann::json in_props);

/// 逃生口：required 只列其中一部分（现有的三个 schema 都不需要，留给将来）。
DOODLELIB_API nlohmann::json schema_object(nlohmann::json in_props, std::vector<std::string> in_required);

/// `{type:array, items, minItems?, maxItems?}`。
DOODLELIB_API nlohmann::json schema_array(
    nlohmann::json in_items, std::optional<std::size_t> in_min = std::nullopt, std::optional<std::size_t> in_max = std::nullopt
);

/// `{type:number, minimum?, maximum?}`。
DOODLELIB_API nlohmann::json schema_number(
    std::optional<double> in_min = std::nullopt, std::optional<double> in_max = std::nullopt
);

DOODLELIB_API nlohmann::json schema_string();
DOODLELIB_API nlohmann::json schema_bool();

/// 自由文本枚举（spec.h 里是 std::string 的那几个字段，取值写在注释里）。
/// schema 与校验**共用同一份常量**，否则「schema 里写了 subtle、校验却不认」不会有任何信号。
DOODLELIB_API nlohmann::json schema_string_enum(std::span<const std::string_view> in_values);

/// C++ 枚举的 schema。用 magic_enum 的名字生成 —— 枚举值存库走的就是这套名字，
/// 让 schema 成为它的投影，而不是另抄一份字面量。
template <typename E>
nlohmann::json schema_enum() {
  nlohmann::json l_enum = nlohmann::json::array();
  for (auto&& l_name : magic_enum::enum_names<E>()) l_enum.emplace_back(l_name);
  return nlohmann::json{{"type", "string"}, {"enum", l_enum}};
}

/// 同上，但剔除一个取值（唯一用途：`variant_axis_kind::none` —— 它是「这是基线」的标记，
/// 不是可搜索的自由度）。
template <typename E>
nlohmann::json schema_enum(E in_exclude) {
  nlohmann::json l_enum   = nlohmann::json::array();
  const auto l_exclude    = magic_enum::enum_name(in_exclude);
  for (auto&& l_name : magic_enum::enum_names<E>()) {
    if (l_name == l_exclude) continue;
    l_enum.emplace_back(l_name);
  }
  return nlohmann::json{{"type", "string"}, {"enum", l_enum}};
}

/// 定长 N 元数字数组（interval_t / rgb 用）。与 interval_t::from_json 的校验一致：
/// 否则模型给个 3 元数组，入库前会在 from_json 里抛 type_error(302)。
DOODLELIB_API nlohmann::json schema_fixed_number_array(std::size_t in_size);

DOODLELIB_API nlohmann::json schema_string_array();

// ===========================================================================
// 3.2 严格读取器（抛 doodle_error{bad_gateway}）
// ===========================================================================
//
// 用在「模型给什么就用什么」的那一段（候选基线 rig 的物化）。strict schema 下缺字段本该
// 不可能发生；发生了说明模型没遵守契约，静默取默认值会把「模型没按要求输出」变成看不出
// 来的偏移。错误消息统一为：
//   "<seam>：字段 <path> 缺失或类型不符合（期望 <期望>）"

DOODLELIB_API nlohmann::json require_object(const nlohmann::json& in_root, std::string_view in_seam, std::string_view in_path);
DOODLELIB_API nlohmann::json require_array(const nlohmann::json& in_root, std::string_view in_seam, std::string_view in_path);
DOODLELIB_API double require_number(const nlohmann::json& in_root, std::string_view in_seam, std::string_view in_path);
DOODLELIB_API std::string require_string(const nlohmann::json& in_root, std::string_view in_seam, std::string_view in_path);
DOODLELIB_API bool require_bool(const nlohmann::json& in_root, std::string_view in_seam, std::string_view in_path);
DOODLELIB_API std::array<double, 3> require_vec3(const nlohmann::json& in_root, std::string_view in_seam, std::string_view in_path);

template <typename E>
E require_enum(const nlohmann::json& in_root, std::string_view in_seam, std::string_view in_path) {
  const auto l_name = require_string(in_root, in_seam, in_path);
  const auto l_v    = magic_enum::enum_cast<E>(l_name);
  DOODLE_CHICK_HTTP(
      l_v.has_value(), bad_gateway, "{}：字段 {} 的值 \"{}\" 不是合法的 {}", in_seam, in_path, l_name,
      magic_enum::enum_type_name<E>()
  );
  return *l_v;
}

/// 自由文本枚举的严格读取：不在白名单内即抛（与宽松的 opt_one_of 是同一份白名单常量）。
DOODLELIB_API std::string require_one_of(
    const nlohmann::json& in_root, std::string_view in_seam, std::string_view in_path,
    std::span<const std::string_view> in_allowed
);

// ===========================================================================
// 3.3 宽松读取器（std::optional / 空容器，永不抛）
// ===========================================================================
//
// 这是 spec 接缝**有意为之**的宽容策略，不是缺陷：读不出就保留统计推导值，并由
// put_evidence(..., evidence_source::default_value, ...) 留痕。语义与重构前逐条一致。

/// 非 object / 非该键 / 该键不是 object → nullopt。
DOODLELIB_API std::optional<nlohmann::json> opt_object(const nlohmann::json& in_parent, std::string_view in_key);
/// 非字符串 **或空串** → nullopt（空串按「没给」处理，与重构前一致）。
DOODLELIB_API std::optional<std::string> opt_string(const nlohmann::json& in_parent, std::string_view in_key);
DOODLELIB_API std::optional<bool> opt_bool(const nlohmann::json& in_parent, std::string_view in_key);
DOODLELIB_API std::optional<double> opt_number(const nlohmann::json& in_parent, std::string_view in_key);
/// 恰好 2 元的数字数组 → interval_t；`[high, low]` 写反时静默交换（比让下游「落点永远不在
/// 区间内」好排查）。
DOODLELIB_API std::optional<interval_t> opt_interval(const nlohmann::json& in_parent, std::string_view in_key);
/// **刻意返回空容器而不是 optional**：调用点一律「有几个用几个」，空容器就是它的语义
/// （重构前即如此，既有测试也钉着它）。
DOODLELIB_API std::vector<std::string> opt_string_array(const nlohmann::json& in_parent, std::string_view in_key);
/// opt_string + 白名单两步合一：值不在白名单内 → nullopt（该字段被弃用，保留默认值）。
DOODLELIB_API std::optional<std::string> opt_one_of(
    const nlohmann::json& in_parent, std::string_view in_key, std::span<const std::string_view> in_allowed
);

// ===========================================================================
// 3.4 架构校验（诊断用，不作准入闸门 —— 见文件头 D-1）
// ===========================================================================

/// 我们自己的三条约定（json-schema-validator 查不到）：
///   * 每个 object 必须 `additionalProperties:false`；
///   * 每个 object 的 `required` 必须**恰好**等于它的属性名集合；
///   * 每个 array 必须有 `items`，以及 minItems <= maxItems。
/// 返回违规描述（空 = 合规）。递归遍历，路径是 `$.a.b[0]` 形状。
DOODLELIB_API std::vector<std::string> check_schema_consistency(const nlohmann::json& in_schema);

/// 库校验：schema 本身是不是合法 draft-7。**失败 = 我们的 bug，不是模型的错。**
/// 返回违规描述（空 = 合规）；构造 validator 抛出的异常也转成一条字符串，本函数永不抛。
DOODLELIB_API std::vector<std::string> validate_schema_is_wellformed(
    const nlohmann::json& in_schema, std::string_view in_seam
);

/// 库校验：模型输出对我们 schema 的**全部**违规，每条形如 `/lights/3/softness: <原因>`
/// （json pointer）。用 basic_error_handler 的派生类一次收齐，而不是读到第一个坏字段就停。
/// 本函数永不抛（异常同样转成一条字符串）。
DOODLELIB_API std::vector<std::string> validate_model_output(
    const nlohmann::json& in_value, const nlohmann::json& in_schema, std::string_view in_seam
);

/// 把违规列表拼成一行日志用文本：只取前 `in_max` 条，末尾标注被省略的条数。
/// 三个接缝各写一遍会漂移（一个截 3 条、另一个截 5 条），所以收在这里。
DOODLELIB_API std::string join_violations(const std::vector<std::string>& in_violations, std::size_t in_max = 3);

}  // namespace doodle::ai_light::json_val
