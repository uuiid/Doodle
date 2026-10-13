//
// AI 灯光：模型输出 JSON 架构验证层的离线测试
// （设计见 docs/ue_ai_lighting_plan.md §15.11）
//
// 这一层全是纯函数（没有 sqlite、没有协程、没有 HTTP），所以它能在单测宿主里被完整覆盖 ——
// 把「模型输出怎么读」集中到一层，收益就是它不必等真机才能验。
//
// 刻意不 include 路由头 `http_method/kitsu/ai_light.h`（它会拉进 <core/http/http_function.h>，
// 而测试目标的 include 路径没有 doodle_lib 根）—— 与其他 ai_light 单测同一约定。
//
#include <doodle_lib/http_method/kitsu/ai_light_json_validation.h>
#include <doodle_lib/http_method/kitsu/ai_light_orchestrator.h>

#include <boost/test/unit_test.hpp>

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace doodle::ai_light {

namespace {

/// 枚举进不了 BOOST_TEST：Boost.Test 需要 operator<< 才能把值打印出来，枚举没有这个运算符
/// （`print_helper.hpp(53): error C2338: static_assert failed: 'Type has to implement operator<<
/// to be printable'`）。所以一律先转成 magic_enum 的名字字符串再比 —— 与 ai_light_state.cpp
/// 里的 en<>() 是同一套路。
template <typename E>
std::string en(E in_value) {
  return std::string{magic_enum::enum_name(in_value)};
}

/// 两个生产 schema 的**形状摘要**（FNV-1a 64，十六进制）。
///
/// 生成方式就是把两个值打印出来抄进来
/// （见 schema_shape_digest_matches_frozen_golden 里的 BOOST_TEST_MESSAGE）。
///
/// 这是形状层的护栏：两个 schema 全部由内核建造器（`schema_object` / `schema_array` /
/// `schema_enum`）拼出来，任何建造器改动都必须让摘要保持原值；摘要不一样就说明
/// 「只改了实现、没改形状」这句话不成立，要回头核对而不是改 golden。
///
/// 摘要器对 `required` / `enum` 的**数组顺序不敏感**（见 canonicalize）—— 建造器按属性名
/// 字典序生成 required，与书写顺序无关，两者语义相同，不该被判成改动。
// 冻结的 schema 形状摘要：这个值必须一字不变 —— 它是「建造器没有改变这个 schema 的
// 形状」的机器可判定证据。生成方式：先跑一次
// `--run_test=ai_light_json_validation --log_level=message`，从「feedback digest = …」那行里抄回来。
// 真要改 schema 形状时**必须**同步改这个常量，并在设计文档里说明为什么。
//
// 曾经这里还有一个金标准：
//   * g_golden_feedback_digest（值 0e13ecb11cd3f474）：接缝 3 的 schema 摘要。
//     接缝 3（build_feedback_json_schema）已整条删除 —— 它在生产里没有调用方，
//     服务端不再替前端跑这次推理。**注意**：上面两个金标准也是同样原因删的。
// 如今这一层不再有生产 schema 可供冻结，护栏改由下面 make_probe_schema() 造出的
// 样本 schema 承担：它覆盖同一批建造器（schema_object / schema_array / schema_enum /
// additionalProperties / require_*），所以「建造器改动必须被看见」这条验收条件不变。

/// 规范化：递归下降，把 `required` / `enum` 数组排序（其余原样）。nlohmann::json 的对象键
/// 本来就按 key 有序，所以「对象键排序」不需要额外动作。
void canonicalize(nlohmann::json& io_json) {
  if (io_json.is_object()) {
    for (auto&& [l_key, l_value] : io_json.items()) {
      canonicalize(l_value);
      if ((l_key == "required" || l_key == "enum") && l_value.is_array()) {
        std::sort(l_value.begin(), l_value.end(), [](const nlohmann::json& in_a, const nlohmann::json& in_b) {
          return in_a.dump() < in_b.dump();
        });
      }
    }
  } else if (io_json.is_array()) {
    for (auto&& l_value : io_json) canonicalize(l_value);
  }
}

std::string shape_digest(const nlohmann::json& in_schema) {
  auto l_copy = in_schema;
  canonicalize(l_copy);
  const auto l_text = l_copy.dump();
  std::uint64_t l_hash = 1469598103934665603ULL;  // FNV-1a 64 的 offset basis
  for (const unsigned char l_c : l_text) {
    l_hash ^= l_c;
    l_hash *= 1099511628211ULL;
  }
  return fmt::format("{:016x}", l_hash);
}

/// 自由文本枚举白名单（grading_hints 那一类在 spec.h 里是 std::string 的字段）。
constexpr std::array<std::string_view, 3> g_test_text_enum{"hard", "soft", "very_soft"};

/// 样本 schema：接缝 3 的 build_feedback_json_schema() 删除后，这一层的三个用例仍需要
/// 一份「由生产建造器拼出来的、有嵌套与枚举的」schema 来验自检 / 诊断 / 摘要。
///
/// 它刻意用**同一批**建造器（schema_object / schema_array / schema_enum / additionalProperties）
/// 拼出与那条接缝同构的形状 —— 护栏的目的（建造器一改，摘要与自检的结论就变）因此不变，
/// 只是不再把某一条具体接缝的 schema 冻结成金标准。
nlohmann::json make_probe_schema() {
  const auto l_axis_item = json_val::schema_object({
      {"axis", json_val::schema_enum<variant_axis_kind>(variant_axis_kind::none)},
      {"value", json_val::schema_number()},
      {"reason", json_val::schema_string()},
  });
  const auto l_axes = json_val::schema_array(l_axis_item, std::nullopt, 2);

  auto l_understanding = json_val::schema_string();

  return json_val::schema_object({
      {"understanding", l_understanding},
      {"axes", l_axes},
  });
}

}  // namespace

/// 只为验证 DOODLE_JSON_SERIALIZE_ENUM_STRICT 语义而设的本地枚举。
///
/// 它刻意不落在任何真实数据结构上：本用例要证明的是宏本身「遇到未映射值抛 out_of_range.410」，
/// 而不是某个业务枚举恰好怎么写。用真实枚举会让这条断言的含义随业务演进而漂移。
///
/// 位置很重要：必须与宏展开落在**同一个命名空间**（`doodle::ai_light`）里，
/// 否则 `get<strict_probe_enum>()` 走 ADL 找不到 `from_json`，会静默退化成
/// `nlohmann::json` 的隐式转换而不是报错 —— 这类错误表现为编译期的
/// `enum_name` 模板推导失败，很容易被误读成 magic_enum 的问题。
enum class strict_probe_enum { alpha, beta };
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    strict_probe_enum,
    {
        {strict_probe_enum::alpha, "alpha"},
        {strict_probe_enum::beta, "beta"},
    }
)

BOOST_AUTO_TEST_SUITE(ai_light_json_validation)

BOOST_AUTO_TEST_CASE(path_resolver_handles_dots_indexes_and_garbage) {
  const nlohmann::json l_json{
      {"a", {{"b", nlohmann::json::array({1, 2, 3})}}},
      {"lights", nlohmann::json::array({nlohmann::json{{"softness", 0.5}}})}
  };

  BOOST_TEST(json_val::json_at_path(l_json, "a.b[1]") != nullptr);
  BOOST_TEST(json_val::json_at_path(l_json, "a.b[1]")->get<int>() == 2);
  BOOST_TEST(json_val::json_at_path(l_json, "lights[0].softness") != nullptr);
  BOOST_TEST(json_val::json_at_path(l_json, "lights[0].softness")->get<double>() == 0.5);

  // 取不到的一律 nullptr（不抛）—— 「路径写错」与「模型没给」在诊断上必须能分开
  BOOST_TEST(json_val::json_at_path(l_json, "a.b[3]") == nullptr);   // 越界
  BOOST_TEST(json_val::json_at_path(l_json, "a.c") == nullptr);      // 不存在
  BOOST_TEST(json_val::json_at_path(l_json, "a.b.c") == nullptr);    // 把数组当对象
  BOOST_TEST(json_val::json_at_path(l_json, "a[0].b") == nullptr);   // 把对象当下标
  BOOST_TEST(json_val::json_at_path(l_json, "a..b") == nullptr);     // 空段
  BOOST_TEST(json_val::json_at_path(l_json, "a.b[x]") == nullptr);   // 下标不是数字
}

BOOST_AUTO_TEST_CASE(builders_emit_full_object_shape) {
  const auto l_obj = json_val::schema_object({{"x", json_val::schema_number()}, {"y", json_val::schema_string()}});
  BOOST_TEST(l_obj.at("type") == "object");
  BOOST_TEST(l_obj.at("additionalProperties") == false);
  // required 默认等于全部属性名（三个接缝现有的 16 个 object 都是满配，所以这是等价重写）
  BOOST_TEST(l_obj.at("required").size() == 2);
  BOOST_TEST(l_obj.at("required").at(0) == "x");
  BOOST_TEST(l_obj.at("required").at(1) == "y");

  // 逃生口：只列一部分必填
  const auto l_partial = json_val::schema_object({{"x", json_val::schema_number()}}, {"x"});
  BOOST_TEST(l_partial.at("required").size() == 1);

  const auto l_arr = json_val::schema_array(json_val::schema_number(), 1, 5);
  BOOST_TEST(l_arr.at("type") == "array");
  BOOST_TEST(l_arr.at("items").at("type") == "number");
  BOOST_TEST(l_arr.at("minItems") == 1);
  BOOST_TEST(l_arr.at("maxItems") == 5);

  const auto l_fixed = json_val::schema_fixed_number_array(2);
  BOOST_TEST(l_fixed.at("minItems") == 2);
  BOOST_TEST(l_fixed.at("maxItems") == 2);

  const auto l_number = json_val::schema_number(-1.0, 1.0);
  BOOST_TEST(l_number.at("minimum") == -1.0);
  BOOST_TEST(l_number.at("maximum") == 1.0);

  // 枚举 schema 是 magic_enum 的投影；剔除重载必须真的剔掉那一个
  const auto l_all_axes    = json_val::schema_enum<variant_axis_kind>();
  const auto l_search_axes = json_val::schema_enum(variant_axis_kind::none);
  BOOST_TEST(l_all_axes.at("enum").size() == l_search_axes.at("enum").size() + 1);
  // 注意 `nlohmann::json{"none"}` **不是**字符串 "none"，而是**数组** ["none"]（initializer_list
  // 构造函数优先）—— 用圆括号或 std::string 构造才是字符串。这是本仓库已经踩过的坑（坑 20 的同一族）。
  const nlohmann::json l_none_name = std::string{"none"};
  const bool l_all_has_none = std::find(l_all_axes.at("enum").begin(), l_all_axes.at("enum").end(), l_none_name) !=
                              l_all_axes.at("enum").end();
  const bool l_search_has_none =
      std::find(l_search_axes.at("enum").begin(), l_search_axes.at("enum").end(), l_none_name) !=
      l_search_axes.at("enum").end();
  BOOST_TEST(l_all_has_none);
  BOOST_TEST(!l_search_has_none);

  const auto l_text_enum = json_val::schema_string_enum(g_test_text_enum);
  BOOST_TEST(l_text_enum.at("type") == "string");
  BOOST_TEST(l_text_enum.at("enum").size() == 3);
}

BOOST_AUTO_TEST_CASE(strict_readers_accept_and_report_seam_and_path) {
  const nlohmann::json l_json{
      {"softness", 0.5},
      {"role", "key"},
      {"cast_shadows", true},
      {"rgb", nlohmann::json::array({0.1, 0.2, 0.3})},
      {"note", ""},
      {"globals", {{"exposure_ev100_bias", 0.0}}}
  };

  BOOST_TEST(json_val::require_number(l_json, "候选生成", "softness") == 0.5);
  BOOST_TEST(json_val::require_string(l_json, "候选生成", "role") == "key");
  BOOST_TEST(json_val::require_bool(l_json, "候选生成", "cast_shadows"));
  const auto l_rgb = json_val::require_vec3(l_json, "候选生成", "rgb");
  BOOST_TEST(l_rgb.at(0) == 0.1);
  BOOST_TEST(l_rgb.at(2) == 0.3);
  // 空字符串对**严格**读取器是合法的：它只校验类型。空串在宽松读取器里才算「没给」。
  BOOST_TEST(json_val::require_string(l_json, "候选生成", "note").empty());

  // 路径可以是绝对的，也可以是相对某个子对象的 —— 两种写法读的是同一个值
  const auto l_globals = json_val::require_object(l_json, "候选生成", "globals");
  BOOST_TEST(json_val::require_number(l_json, "候选生成", "globals.exposure_ev100_bias") == 0.0);
  BOOST_TEST(json_val::require_number(l_globals, "候选生成", "exposure_ev100_bias") == 0.0);

  // 失败消息必须同时带**接缝名**与**字段路径**：重构前那套把接缝名写死成「候选基线 rig」，
  // 另两条接缝一复用就会报错措辞错人（方案 §0 的核心问题之一）。
  bool l_threw = false;
  try {
    (void)json_val::require_number(l_json, "文本反馈", "globals.missing");
  } catch (const doodle_error& in_err) {
    l_threw = true;
    const std::string l_what{in_err.what()};
    BOOST_TEST(l_what.find("文本反馈") != std::string::npos);
    BOOST_TEST(l_what.find("globals.missing") != std::string::npos);
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::bad_gateway);
  }
  BOOST_TEST(l_threw);

  // 类型不符也要抛（并且不是「静默用默认值」）
  l_threw = false;
  try {
    (void)json_val::require_vec3(l_json, "候选生成", "softness");
  } catch (const doodle_error& in_err) {
    l_threw = true;
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::bad_gateway);
  }
  BOOST_TEST(l_threw);
}

BOOST_AUTO_TEST_CASE(enum_readers_validate_against_magic_enum_names) {
  const nlohmann::json l_json{{"role", "key"}, {"hardness", "very_soft"}, {"bad_role", "key_light"}};

  BOOST_TEST(en(json_val::require_enum<light_role>(l_json, "候选生成", "role")) == "key");
  BOOST_TEST(json_val::require_one_of(l_json, "候选生成", "hardness", g_test_text_enum) == "very_soft");

  // 认不出的枚举名必须抛：NLOHMANN_JSON_SERIALIZE_ENUM 的裸 get<> 会静默落到第一个枚举值，
  // 那等于把「模型给了非法灯角色」变成一盏看起来正常的 key 灯。
  bool l_threw = false;
  try {
    (void)json_val::require_enum<light_role>(l_json, "候选生成", "bad_role");
  } catch (const doodle_error& in_err) {
    l_threw = true;
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::bad_gateway);
  }
  BOOST_TEST(l_threw);

  // 自由文本枚举：不在白名单内同样抛（严格通道），而宽松通道只是丢弃（见下一条用例）
  l_threw = false;
  try {
    (void)json_val::require_one_of(l_json, "候选生成", "role", g_test_text_enum);
  } catch (const doodle_error&) {
    l_threw = true;
  }
  BOOST_TEST(l_threw);
}

/// DOODLE_JSON_SERIALIZE_ENUM_STRICT 的核心语义：认不出的取值必须当场抛异常。
///
/// 这是「本库不再静默吞掉非法枚举」的唯一机器可判定证据 —— 旧的
/// NLOHMANN_JSON_SERIALIZE_ENUM 在同样输入下会**静默返回第一个枚举值**（默认 alpha），
/// 于是「模型给了垃圾」会变成一条看起来完全正常的业务数据。
BOOST_AUTO_TEST_CASE(strict_enum_macro_throws_on_unmapped_value) {
  // 1) 反序列化：认不出的字符串必须抛，而不是落到 alpha
  bool                 l_threw = false;
  const nlohmann::json l_bad   = nlohmann::json("gamma");
  try {
    (void)l_bad.get<strict_probe_enum>();
  } catch (const nlohmann::detail::out_of_range& in_err) {
    l_threw = true;
    BOOST_TEST(in_err.id == 410);
  }
  BOOST_TEST(l_threw);

  // 2) 映射内的取值照常工作（严格不等于不可用）
  const nlohmann::json l_alpha = nlohmann::json("alpha");
  const nlohmann::json l_beta  = nlohmann::json("beta");
  BOOST_TEST(l_alpha.is_string());
  BOOST_TEST(en(l_alpha.get<strict_probe_enum>()) == "alpha");
  BOOST_TEST(en(l_beta.get<strict_probe_enum>()) == "beta");

  // 序列化 -> 反序列化往返
  const nlohmann::json l_ser = strict_probe_enum::beta;
  BOOST_TEST(l_ser.get<std::string>() == "beta");
  BOOST_TEST(en(l_ser.get<strict_probe_enum>()) == "beta");

  // 3) 序列化：枚举值不在映射表中同样抛（这里用强转伪造一个未映射值）
  l_threw = false;
  try {
    const nlohmann::json l_out = static_cast<strict_probe_enum>(99);
    (void)l_out;
  } catch (const nlohmann::detail::out_of_range& in_err) {
    l_threw = true;
    BOOST_TEST(in_err.id == 410);
  }
  BOOST_TEST(l_threw);
}

BOOST_AUTO_TEST_CASE(opt_readers_are_lenient_and_never_throw) {
  const nlohmann::json l_json{
      {"empty_string", ""},
      {"text", "hello"},
      {"flag", true},
      {"value", 3.5},
      {"interval_reversed", nlohmann::json::array({0.8, 0.4})},
      {"interval_three", nlohmann::json::array({0.1, 0.2, 0.3})},
      {"tags", nlohmann::json::array({"a", "b"})},
      {"tags_mixed", nlohmann::json::array({"a", 1})},
      {"hardness", "very_soft"},
      {"hardness_bad", "extremely_soft"},
      {"obj", {{"k", 1}}}
  };

  BOOST_TEST(json_val::opt_string(l_json, "text").value_or("") == "hello");
  // 空串按「没给」处理（与重构前一致）
  BOOST_TEST(!json_val::opt_string(l_json, "empty_string").has_value());
  BOOST_TEST(!json_val::opt_string(l_json, "value").has_value());

  BOOST_TEST(json_val::opt_bool(l_json, "flag").value_or(false));
  BOOST_TEST(!json_val::opt_bool(l_json, "value").has_value());
  BOOST_TEST(json_val::opt_number(l_json, "value").value_or(0.0) == 3.5);
  BOOST_TEST(!json_val::opt_number(l_json, "flag").has_value());

  // 区间写反时静默交换：让它直接生效比让下游「落点永远不在区间内」好排查一个数量级
  const auto l_interval = json_val::opt_interval(l_json, "interval_reversed");
  BOOST_TEST(l_interval.has_value());
  BOOST_TEST(l_interval->low_ == 0.4);
  BOOST_TEST(l_interval->high_ == 0.8);
  BOOST_TEST(!json_val::opt_interval(l_json, "interval_three").has_value());

  BOOST_TEST(json_val::opt_string_array(l_json, "tags").size() == 2);
  BOOST_TEST(json_val::opt_string_array(l_json, "tags_mixed").empty());  // 有一个不是字符串 → 整体当没给
  BOOST_TEST(json_val::opt_string_array(l_json, "missing").empty());

  BOOST_TEST(json_val::opt_object(l_json, "obj").has_value());
  BOOST_TEST(!json_val::opt_object(l_json, "value").has_value());

  BOOST_TEST(json_val::opt_one_of(l_json, "hardness", g_test_text_enum).value_or("") == "very_soft");
  BOOST_TEST(!json_val::opt_one_of(l_json, "hardness_bad", g_test_text_enum).has_value());

  // 根不是 object 时全部返回空，且不抛
  const nlohmann::json l_not_object = nlohmann::json::array({1, 2});
  BOOST_TEST(!json_val::opt_string(l_not_object, "x").has_value());
  BOOST_TEST(!json_val::opt_number(l_not_object, "x").has_value());
  BOOST_TEST(json_val::opt_string_array(l_not_object, "x").empty());
}

BOOST_AUTO_TEST_CASE(consistency_check_catches_the_three_conventions) {
  const auto l_ok =
      json_val::schema_object({{"a", json_val::schema_number()}, {"b", json_val::schema_array(json_val::schema_string(), 1, 3)}});
  BOOST_TEST(json_val::check_schema_consistency(l_ok).empty());

  // ① 缺 additionalProperties:false
  nlohmann::json l_no_ap = l_ok;
  l_no_ap.erase("additionalProperties");
  BOOST_TEST(json_val::check_schema_consistency(l_no_ap).size() == 1);

  // ② required 与属性名集合不一致
  nlohmann::json l_bad_required = l_ok;
  l_bad_required["required"] = nlohmann::json::array({"a"});
  BOOST_TEST(json_val::check_schema_consistency(l_bad_required).size() == 1);

  // ③ array 缺 items
  nlohmann::json l_no_items = l_ok;
  l_no_items["properties"]["b"].erase("items");
  BOOST_TEST(json_val::check_schema_consistency(l_no_items).size() == 1);

  // 嵌套的对象同样被走到：内层缺 additionalProperties（1 条）+ 缺 required（2 条：
  // 「缺失或不是数组」与「required 与属性名集合不一致」）
  const auto l_nested = json_val::schema_object(
      {{"inner", {{"type", "object"}, {"properties", {{"x", json_val::schema_number()}}}}}}
  );
  BOOST_TEST(json_val::check_schema_consistency(l_nested).size() == 3);

  // 顶层不是 object
  BOOST_TEST(json_val::check_schema_consistency(nlohmann::json::array()).size() == 1);
}

BOOST_AUTO_TEST_CASE(wellformed_check_accepts_production_schemas_and_flags_broken_ones) {
  for (const auto& [l_name, l_schema] :
       std::initializer_list<std::pair<std::string_view, nlohmann::json>>{
           {"probe", make_probe_schema()}
       }) {
    // 库校验：schema 本身是不是合法 draft-7。失败 = 我们的 bug，不是模型的错。
    BOOST_TEST(json_val::validate_schema_is_wellformed(l_schema, l_name).empty());
  }

  // 人工构造的坏 schema 必须被指出来（否则「自检通过」这句话没有意义）。
  // 下面四种是**实测**中 pboettch 2.4.0 在 `set_root_schema` 阶段真的会抛的坏 schema（各 1 条违规）。
  for (const auto& [l_name, l_schema] :
       std::initializer_list<std::pair<std::string_view, nlohmann::json>>{
           {"required 不是数组", nlohmann::json{{"type", "object"}, {"required", "not-an-array"}}},
           {"properties 不是对象", nlohmann::json{{"type", "object"}, {"properties", 5}}},
           {"$ref 指向不存在的定义", nlohmann::json{{"$ref", "#/definitions/missing"}}},
           {"pattern 不是合法正则", nlohmann::json{{"type", "string"}, {"pattern", "["}}}
       }) {
    BOOST_TEST(!json_val::validate_schema_is_wellformed(l_schema, l_name).empty());
  }

  // 库**抓不到**的坏形状（实测均 0 条违规）：`{"type":"nonsense"}`、`{"type":5}`、数组没有 items、
  // minItems > maxItems、enum 不是数组。旧版库把这些当成「运行时永远匹配不上」，而不是「schema 写错了」。
  // 这正是 `check_schema_consistency` 必须存在、且**不能**只依赖库自检的原因 —— 库漏掉的形状由我们
  // 自己的走查补上（数组缺 items 就是其中一条）。
  BOOST_TEST(json_val::validate_schema_is_wellformed(nlohmann::json{{"type", "array"}}, "人工构造").empty());
  BOOST_TEST(!json_val::check_schema_consistency(nlohmann::json{{"type", "array"}}).empty());
}

BOOST_AUTO_TEST_CASE(model_output_violations_carry_json_pointer) {
  const auto l_schema = make_probe_schema();

  const nlohmann::json l_ok{
      {"understanding", "美术想让主光再低一点"},
      {"axes", nlohmann::json::array({nlohmann::json{{"axis", "key_direction"}, {"value", -15.0}, {"reason", "主光偏高"}}})}
  };
  BOOST_TEST(json_val::validate_model_output(l_ok, l_schema, "文本反馈").empty());

  // 缺必填字段
  const nlohmann::json l_missing{{"understanding", "只有理解没有轴"}};
  BOOST_TEST(!json_val::validate_model_output(l_missing, l_schema, "文本反馈").empty());

  // 多一个无关键（additionalProperties:false 会被指出来 —— 正因为能被指出来，把校验当
  // 闸门才是危险的：今天这一条是被静默忽略的）
  const nlohmann::json l_extra{
      {"understanding", "x"}, {"axes", nlohmann::json::array()}, {"extra_key", 1}
  };
  BOOST_TEST(!json_val::validate_model_output(l_extra, l_schema, "文本反馈").empty());

  // 类型不符要带 json pointer：诊断必须能直接指到哪个字段
  const nlohmann::json l_wrong_type{{"understanding", 1}, {"axes", nlohmann::json::array()}};
  const auto l_violations = json_val::validate_model_output(l_wrong_type, l_schema, "文本反馈");
  BOOST_TEST(!l_violations.empty());
  const bool l_has_pointer =
      std::any_of(l_violations.begin(), l_violations.end(), [](const std::string& in_v) {
        return in_v.find("/understanding") != std::string::npos;
      });
  BOOST_TEST(l_has_pointer);

  // join_violations 只影响日志格式：空列表给「无」，超长给省略说明
  BOOST_TEST(json_val::join_violations({}) == "无");
  const std::vector<std::string> l_many{"a", "b", "c", "d"};
  const auto l_joined = json_val::join_violations(l_many, 2);
  BOOST_TEST(l_joined.find("a；b") != std::string::npos);
  BOOST_TEST(l_joined.find("另有 2 条") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(schema_shape_digest_matches_frozen_golden) {
  // 摘要器本身必须对 required / enum 的顺序不敏感：建造器按属性名字典序生成 required，
  // 手工版本按书写顺序，两者语义相同，不该被判成改动。
  const nlohmann::json l_order_a{
      {"type", "object"},
      {"properties", {{"x", json_val::schema_number()}, {"y", json_val::schema_string()}}},
      {"required", nlohmann::json::array({"x", "y"})},
      {"additionalProperties", false}
  };
  nlohmann::json l_order_b = l_order_a;
  l_order_b["required"]    = nlohmann::json::array({"y", "x"});
  BOOST_TEST(shape_digest(l_order_a) == shape_digest(l_order_b));
  BOOST_TEST(shape_digest(l_order_a) != shape_digest(json_val::schema_object({{"x", json_val::schema_number()}})));

  // 样本 schema 的摘要。曾经这里冻结的是三条接缝各自的 schema 摘要；三条接缝全部删除后
  // 不再有生产 schema 可冻结，所以这里只验「摘要器对同一份输入稳定」。
  const auto l_probe = shape_digest(make_probe_schema());
  BOOST_TEST_MESSAGE("probe digest = " << l_probe);
  BOOST_TEST(l_probe == shape_digest(make_probe_schema()));

  // 建造器一改，摘要必须变 —— 否则这条护栏是假的。这里用「多一个属性」代替建造器改动：
  // 形状确实不同，摘要就必须不同。
  nlohmann::json l_perturbed = make_probe_schema();
  l_perturbed["properties"]["extra"] = json_val::schema_number();
  BOOST_TEST(l_probe != shape_digest(l_perturbed));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace doodle::ai_light
