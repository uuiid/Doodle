//
// AI 灯光：接缝 2 的离线可测部件（docs/ue_ai_lighting_plan.md §15.8）
//
// generate_candidates 本体要联网（火山方舟 Responses）且需要 SSL context，单测宿主两样都没有，
// 所以这里盯的是**离线那一半** —— 组装推理信封 + 把「基线 rig + 轴计划」映射成 6 个候选。
//
// 这份测试钉的是三件事：
//   1) 服务端钉死的字段（身份、schema_version、exposure.method）**不因模型输出而改变**；
//   2) 6 个候选是**确定性 OFAT 展开**的结果：候选 0 是基线，其余每个只沿一条轴变化，
//      且灯集合与基线逐个相同（§6.6 的护栏是构造性保证，不是事后校验）；
//   3) 会导致「静默产生 6 张一样的图」或「整轮建在非法值上」的输入必须当场失败，
//      而不是被默认值吞掉。
//

// 与 ai_light_spec_prompt.cpp 同一个理由，刻意**不**包含路由头 http_method/kitsu/ai_light.h
// （它 include 了 <core/http/http_function.h>，而测试目标的 include 路径里没有 doodle_lib 根）。
#include <doodle_core/configure/static_value.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_light/ai_light.h>
#include <doodle_lib/core/core_set.h>

#include <doodle_lib/http_client/seedance2_client.h>
#include <doodle_lib/http_method/kitsu/ai_light_orchestrator.h>

#include <boost/test/unit_test.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <string>
#include <vector>

namespace doodle::ai_light {
namespace {

// ---------------------------------------------------------------------------
// 构造助手
// ---------------------------------------------------------------------------

lighting_spec_t make_spec() {
  lighting_spec_t l_spec{};
  l_spec.spec_id_    = core_set::get_set().get_uuid();
  l_spec.session_id_ = core_set::get_set().get_uuid();

  l_spec.intent_.atmosphere_    = std::vector<std::string>{"霓虹", "潮湿"};
  l_spec.intent_.time_of_day_   = "夜";
  l_spec.intent_.weather_       = "小雨";
  l_spec.intent_.setting_       = "雨夜街道";
  l_spec.intent_.story_context_ = "主角独行";

  character_personality_t l_person{};
  l_person.subject_                          = "主角";
  l_person.trait_                            = "阴郁内向";
  l_person.lighting_language_.hardness_      = "hard";
  l_person.lighting_language_.position_bias_ = "side_high";
  l_person.lighting_language_.kelvin_bias_   = "cool";
  l_person.lighting_language_.key_fill_ratio_ = interval_t{6.0, 10.0};
  l_spec.intent_.character_personality_       = std::vector<character_personality_t>{l_person};

  l_spec.targets_.contrast_             = interval_t{0.62, 0.72};
  l_spec.targets_.white_balance_kelvin_ = interval_t{4000.0, 4200.0};
  l_spec.reference_stats_.per_dimension_confidence_["contrast"] = 0.8;
  return l_spec;
}

ai_light_preset make_preset(const std::string& in_name, const std::vector<std::string>& in_tags, bool in_enabled) {
  ai_light_preset l_preset{};
  l_preset.name_    = in_name;
  l_preset.tags_    = in_tags;
  l_preset.enabled_ = in_enabled;
  lighting_targets_t l_target{};
  l_target.contrast_             = interval_t{0.60, 0.80};
  l_target.white_balance_kelvin_ = interval_t{3000.0, 3800.0};
  l_preset.set_stats_target(l_target);
  return l_preset;
}

candidate_generation_request_t make_request() {
  candidate_generation_request_t l_req{};
  l_req.session_id_     = core_set::get_set().get_uuid();
  l_req.project_id_     = core_set::get_set().get_uuid();
  l_req.operator_id_    = core_set::get_set().get_uuid();
  l_req.iteration_id_   = core_set::get_set().get_uuid();
  l_req.iteration_      = 1;
  l_req.spec_           = make_spec();
  l_req.prompt_text_    = "雨夜霓虹街道，人物阴郁内向";
  l_req.baseline_level_ = "/Game/Test/RenderMap";
  l_req.presets_        = std::vector<ai_light_preset>{
      make_preset("夜景霓虹", {"霓虹"}, true),
      make_preset("三点布光", {"棚拍"}, true),
      make_preset("停用预设", {"霓虹"}, false),
  };
  return l_req;
}

/// 一份「模型合法输出」的最小样本：一盏主光 + 一盏补光 + 一盏环境灯，
/// 轴计划是 key_fill_ratio 三档 + kelvin 两档（1 + 3 + 2 == 6）。
nlohmann::json make_light_json(const char* in_role, double in_azimuth_deg) {
  return nlohmann::json{
      {"role", in_role},
      {"type", "spot"},
      {"azimuth_deg", in_azimuth_deg},
      {"elevation_deg", 30.0},
      {"distance_m", 3.0},
      {"intensity_unit", "candela"},
      {"intensity", 1000.0},
      {"color_mode", "temperature"},
      {"kelvin", 5600.0},
      {"rgb", nlohmann::json::array({0.0, 0.0, 0.0})},
      {"filter", ""},
      {"softness", 0.5},
      {"source_radius_cm", 20.0},
      {"cast_shadows", true},
      {"note", ""},
  };
}

nlohmann::json make_globals_json() {
  return nlohmann::json{
      {"exposure_ev100_bias", 0.0},
      {"white_balance_kelvin", 5600.0},
      {"sky_light_intensity", 1.0},
      {"fog_density", 0.0},
      {"grading_contrast", 1.0},
      {"grading_saturation", 1.0},
      {"grading_lift", 0.0},
      {"grading_vignette", 0.0},
      {"grading_bloom", 0.0},
  };
}

nlohmann::json make_axis_json() {
  return nlohmann::json::array({
      nlohmann::json{
          {"axis", "key_fill_ratio"},
          {"steps", nlohmann::json::array({3.0, 6.0, 10.0})},
          {"label", "对比档位"}
      },
      nlohmann::json{{"axis", "kelvin"}, {"steps", nlohmann::json::array({-800.0, 800.0})}, {"label", "冷暖"}},
  });
}

nlohmann::json make_model_json() {
  return nlohmann::json{
      {"strategy", "首轮大跨度探索对比结构。"},
      {"axes", make_axis_json()},
      {"globals", make_globals_json()},
      {"lights",
       nlohmann::json::array({
           make_light_json("key", 45.0),
           make_light_json("fill", -30.0),
           make_light_json("ambient", 0.0),
       })},
  };
}

/// 把 json array 的字符串元素取成 vector<string>，用于集合比较。
std::vector<std::string> to_strings(const nlohmann::json& in_json) {
  std::vector<std::string> l_ret{};
  for (const auto& l_item : in_json) l_ret.emplace_back(l_item.get<std::string>());
  std::sort(l_ret.begin(), l_ret.end());
  return l_ret;
}

/// Boost.Test 打不出普通枚举（`print_helper.hpp: static_assert failed: 'Type has to implement
/// operator<< to be printable'`，§15.7 坑 18）。比较枚举前先套一层转成字符串。
template <typename E>
std::string en(E in_e) {
  return std::string{magic_enum::enum_name(in_e)};
}

/// object 的键名集合（排序后），用于「required 是否覆盖全部 properties」这类断言。
std::vector<std::string> keys_of(const nlohmann::json& in_object) {
  std::vector<std::string> l_ret{};
  for (auto l_it = in_object.begin(); l_it != in_object.end(); ++l_it) l_ret.emplace_back(l_it.key());
  std::sort(l_ret.begin(), l_ret.end());
  return l_ret;
}

}  // namespace

BOOST_AUTO_TEST_SUITE(ai_light_candidates)

// ---------------------------------------------------------------------------
// schema 与信封
// ---------------------------------------------------------------------------

/// 轴枚举必须与 C++ 枚举逐字一致，且**不含 none**：none 是「这是基线」的标记，不是可搜索的自由度。
/// 枚举名对不上的代价是模型给一个合法名字、服务端 enum_cast 失败 → 整轮 502。
BOOST_AUTO_TEST_CASE(axis_enum_excludes_none) {
  const auto l_schema = build_candidates_json_schema();
  const auto l_enum   = l_schema.at("properties").at("axes").at("items").at("properties").at("axis").at("enum");

  const std::string l_none{magic_enum::enum_name(variant_axis_kind::none)};
  std::vector<std::string> l_want{};
  for (auto&& l_name : magic_enum::enum_names<variant_axis_kind>())
    if (l_name != l_none) l_want.emplace_back(std::string{l_name});
  std::sort(l_want.begin(), l_want.end());

  const auto l_got = to_strings(l_enum);
  BOOST_CHECK_EQUAL_COLLECTIONS(l_got.begin(), l_got.end(), l_want.begin(), l_want.end());
}

/// 严格结构化输出的要求：每个 object 都 additionalProperties:false 且 required 覆盖全部 properties。
/// 另外，服务端自己填的字段（身份、关卡、ops、schema_version）**不能**出现在 schema 里 ——
/// 一旦出现，模型就会开始编造它们。
BOOST_AUTO_TEST_CASE(json_schema_is_strict_and_free_of_server_fields) {
  const auto l_schema = build_candidates_json_schema();

  BOOST_CHECK_EQUAL(l_schema.at("type").get<std::string>(), "object");
  BOOST_CHECK_EQUAL(l_schema.at("additionalProperties").get<bool>(), false);
  // 注意：这两个助手都**按值返回 vector**，绝不能把 begin()/end() 直接写进断言 —— 那会取到
  // 两个不同临时对象的迭代器，MSVC 调试迭代器会以
  // `vector(202): Assertion failed: vector iterators incompatible` 当场 abort 整个用例。
  const auto l_prop_keys = keys_of(l_schema.at("properties"));
  const auto l_req_keys  = to_strings(l_schema.at("required"));
  BOOST_CHECK_EQUAL_COLLECTIONS(l_prop_keys.begin(), l_prop_keys.end(), l_req_keys.begin(), l_req_keys.end());

  // 每盏灯的 object 也必须全 required（严格模式下 optional 字段是不允许的）。
  const auto l_light = l_schema.at("properties").at("lights").at("items");
  BOOST_CHECK_EQUAL(l_light.at("additionalProperties").get<bool>(), false);
  const auto l_light_prop_keys = keys_of(l_light.at("properties"));
  const auto l_light_req_keys  = to_strings(l_light.at("required"));
  BOOST_CHECK_EQUAL_COLLECTIONS(
      l_light_prop_keys.begin(), l_light_prop_keys.end(), l_light_req_keys.begin(), l_light_req_keys.end()
  );

  // 服务端字段一个都不该出现。
  const auto l_dump = l_schema.dump();
  for (const char* l_forbidden : {"variant_axis", "baseline_level", "rig_id", "candidate_index", "schema_version", "ops"})
    BOOST_TEST(l_dump.find(l_forbidden) == std::string::npos, l_forbidden << " 不该出现在 schema 里");

  // 灯数量**不设上限**：决策 17 明确「单镜头动态阴影灯上限先不限制」，
  // 在 schema 层加 maxItems 等于偷偷引入一条新的性能约束。
  BOOST_CHECK_EQUAL(l_schema.at("properties").at("lights").contains("maxItems"), false);
}

/// 信封里**不该有**的键：没有 images 就不发 images / image_detail；max_output_tokens 与
/// thinking 交给客户端缺省（前者缺省会截断，后者缺省 disabled）——「不发」才是正确的表达，
/// 给「未指定」的参数塞一个缺省值等于替调用方做了一个它没做的决定（§15.6 坑 21）。
BOOST_AUTO_TEST_CASE(build_request_omits_absent_inputs) {
  const auto l_req     = make_request();
  // 拷贝初始化，**不要**写 nlohmann::json{...}：花括号走 initializer_list 构造，
  // 得到的是单元素数组，下面的 contains 会全部假通过。
  const nlohmann::json l_envelope = build_candidates_request(l_req);
  for (const char* l_key : {"images", "image_detail", "max_output_tokens", "thinking"})
    BOOST_TEST(!l_envelope.contains(l_key), l_key << " 不该被发送");

  const auto l_body = http::seedance2::seedance2_client::build_inference_body(build_candidates_request(l_req));
  BOOST_CHECK_EQUAL(l_body.at("text").at("format").at("name").get<std::string>(), "lighting_candidates");
  BOOST_CHECK_EQUAL(l_body.at("text").at("format").at("type").get<std::string>(), "json_schema");
  BOOST_CHECK_EQUAL(l_body.at("text").at("format").at("strict").get<bool>(), true);
  BOOST_CHECK_CLOSE(l_body.at("temperature").get<double>(), 0.2, 1e-9);

  // 线格式上「不发图」= content 里没有 input_image 项。
  const auto& l_content = l_body.at("input").at(0).at("content");
  for (const auto& l_item : l_content) BOOST_TEST(l_item.at("type").get<std::string>() != "input_image");
  BOOST_CHECK_EQUAL(l_body.contains("max_output_tokens"), false);
}

/// 提示词里必须带上：方案本身、预设（且**停用的不进**）、warm start 的上一轮 rig、
/// 以及美术用滑块指定的方向（那几条轴必须覆盖到）。
BOOST_AUTO_TEST_CASE(input_text_carries_spec_presets_and_feedback) {
  auto l_req       = make_request();
  l_req.iteration_ = 2;
  l_req.base_rig_  = build_base_rig(make_model_json());

  iteration_axes_t l_feedback{};
  l_feedback.axes_ = std::vector<axis_step_t>{axis_step_t{variant_axis_kind::kelvin, {-600.0}, "人工反馈"}};
  l_req.feedback_axes_ = l_feedback;

  const auto l_text = build_candidates_input_text(l_req);
  BOOST_TEST(l_text.find("已确认的灯光方案") != std::string::npos);
  BOOST_TEST(l_text.find("雨夜街道") != std::string::npos);
  BOOST_TEST(l_text.find("阴郁内向") != std::string::npos);
  BOOST_TEST(l_text.find("夜景霓虹") != std::string::npos);
  BOOST_TEST(l_text.find("三点布光") != std::string::npos);
  BOOST_TEST(l_text.find("停用预设") == std::string::npos);  // enabled_ == false 的不进提示词
  BOOST_TEST(l_text.find("warm start") != std::string::npos);
  BOOST_TEST(l_text.find("kelvin = -600") != std::string::npos);
}

// ---------------------------------------------------------------------------
// 默认轴计划与基线 rig 物化
// ---------------------------------------------------------------------------

/// 默认轴计划的唯一硬指标：1 + Σ|steps| == 6，且没有 0 档（0 档等于基线，
/// 会当场产生一个与候选 0 相同的候选）。
BOOST_AUTO_TEST_CASE(default_axis_plan_fills_six_candidates) {
  const auto l_plan = default_axis_plan(make_spec());

  std::size_t l_total = 0;
  for (const auto& l_axis : l_plan.axes_) {
    BOOST_TEST(!l_axis.steps_.empty());
    for (const double l_step : l_axis.steps_) BOOST_CHECK(l_step != 0.0);
    l_total += l_axis.steps_.size();
  }
  BOOST_CHECK_EQUAL(l_total, static_cast<std::size_t>(g_candidate_count) - 1);
  BOOST_TEST(l_plan.strategy_.find("默认轴计划") != std::string::npos);
}

/// 物化：id 由服务端按顺序生成（模型给的 id 可能重名，而 id 是 ops 的对齐键），
/// 曝光**必须**锁成 manual（§7.5：自动曝光会把强度改动吃掉，美术看到的差别会来自测光）。
BOOST_AUTO_TEST_CASE(base_rig_materializes_ids_and_locks_exposure) {
  const auto l_rig = build_base_rig(make_model_json());

  BOOST_CHECK_EQUAL(l_rig.lights_.size(), 3u);
  BOOST_CHECK_EQUAL(l_rig.lights_.at(0).id_, std::string{"L00"});
  BOOST_CHECK_EQUAL(l_rig.lights_.at(1).id_, std::string{"L01"});
  BOOST_CHECK_EQUAL(l_rig.lights_.at(2).id_, std::string{"L02"});
  BOOST_CHECK_EQUAL(std::string{magic_enum::enum_name(l_rig.lights_.at(0).role_)}, std::string{"key"});
  BOOST_CHECK_EQUAL(l_rig.globals_.exposure_.method_, std::string{"manual"});
  BOOST_CHECK_EQUAL(l_rig.schema_version_, std::string{g_rig_schema_version});
  BOOST_CHECK_CLOSE(l_rig.lights_.at(0).color_.kelvin_, 5600.0, 1e-9);
  BOOST_CHECK_EQUAL(l_rig.lights_.at(0).shadows_.cast_, true);
}

/// 非法枚举取值必须当场失败。**不能**靠 NLOHMANN_JSON_SERIALIZE_ENUM 的裸 get<>：
/// 它遇到认不出的字符串不抛、而是静默落到第一个枚举值 —— 那等于把「模型输出了一个
/// 非法灯角色」变成一盏看起来正常的 key 灯，整轮候选都建立在一个错值上。
BOOST_AUTO_TEST_CASE(base_rig_rejects_illegal_values) {
  auto l_bad_role = make_model_json();
  l_bad_role["lights"][0]["role"] = "super_key";
  BOOST_CHECK_THROW(build_base_rig(l_bad_role), doodle_error);

  auto l_bad_enum_color = make_model_json();
  l_bad_enum_color["lights"][0]["color_mode"] = "hue";
  BOOST_CHECK_THROW(build_base_rig(l_bad_enum_color), doodle_error);

  auto l_missing = make_model_json();
  l_missing["globals"].erase("fog_density");
  BOOST_CHECK_THROW(build_base_rig(l_missing), doodle_error);

  auto l_wrong_rgb = make_model_json();
  l_wrong_rgb["lights"][0]["rgb"] = nlohmann::json::array({0.1, 0.2});  // 不是 3 元
  BOOST_CHECK_THROW(build_base_rig(l_wrong_rgb), doodle_error);

  auto l_no_lights = make_model_json();
  l_no_lights["lights"] = nlohmann::json::array();
  BOOST_CHECK_THROW(build_base_rig(l_no_lights), doodle_error);
}

/// F2/F3：颜色的三个分支**按 color_mode 路由，各自严格读**。
/// 重构前 rgb 是「存在才校验」的两段式写法，模型答了 `color_mode = "rgb"` 却没给 rgb 时
/// 会静默用结构体默认色物化 —— 一次明确的协议违规变成一盏看不出来的偏移灯，与
/// 「裸 get<Enum> 静默落值」属于同一类问题。这里把三分支的「缺字段必须抛」与
/// 「给了就按值物化」都钉住（非当前分支的字段缺失/null 仍然容忍，见用例末尾）。
BOOST_AUTO_TEST_CASE(build_base_rig_requires_rgb_when_mode_is_rgb) {
  auto l_rgb_missing = make_model_json();
  l_rgb_missing["lights"][0]["color_mode"] = "rgb";
  l_rgb_missing["lights"][0].erase("rgb");
  BOOST_CHECK_THROW(build_base_rig(l_rgb_missing), doodle_error);

  auto l_rgb_ok = make_model_json();
  l_rgb_ok["lights"][0]["color_mode"] = "rgb";
  l_rgb_ok["lights"][0]["rgb"]        = nlohmann::json::array({0.25, 0.5, 0.75});
  const auto l_rig_rgb                = build_base_rig(l_rgb_ok);
  BOOST_CHECK_CLOSE(l_rig_rgb.lights_.at(0).color_.rgb_.x_, 0.25, 1e-9);
  BOOST_CHECK_CLOSE(l_rig_rgb.lights_.at(0).color_.rgb_.y_, 0.5, 1e-9);
  BOOST_CHECK_CLOSE(l_rig_rgb.lights_.at(0).color_.rgb_.z_, 0.75, 1e-9);

  auto l_kelvin_missing = make_model_json();
  l_kelvin_missing["lights"][0].erase("kelvin");
  BOOST_CHECK_THROW(build_base_rig(l_kelvin_missing), doodle_error);

  auto l_filter_missing = make_model_json();
  l_filter_missing["lights"][0]["color_mode"] = "filter";
  l_filter_missing["lights"][0].erase("filter");
  BOOST_CHECK_THROW(build_base_rig(l_filter_missing), doodle_error);

  auto l_filter_ok = make_model_json();
  l_filter_ok["lights"][0]["color_mode"] = "filter";
  l_filter_ok["lights"][0]["filter"]     = "CTS";
  BOOST_CHECK_EQUAL(build_base_rig(l_filter_ok).lights_.at(0).color_.filter_, std::string{"CTS"});

  // 非当前分支的字段缺失/null 必须继续容忍：schema 里三分支都必填，但提示词明确说了
  // 非本分支填中性占位 —— 这里把 rgb 整个删掉（temperature 分支用不到它）仍然要能物化。
  auto l_other_branch_tolerated = make_model_json();
  l_other_branch_tolerated["lights"][0].erase("rgb");
  BOOST_CHECK_CLOSE(build_base_rig(l_other_branch_tolerated).lights_.at(0).color_.kelvin_, 5600.0, 1e-9);

  // 反向同理：rgb 分支下 kelvin 为 null 不该抛（它不在当前分支的读路径上）。
  auto l_rgb_ignores_kelvin = make_model_json();
  l_rgb_ignores_kelvin["lights"][0]["color_mode"] = "rgb";
  l_rgb_ignores_kelvin["lights"][0]["kelvin"]     = nullptr;
  BOOST_CHECK(build_base_rig(l_rgb_ignores_kelvin).lights_.at(0).color_.mode_ == color_mode::rgb);
}

// ---------------------------------------------------------------------------
// 单轴施加与 OFAT 展开
// ---------------------------------------------------------------------------

/// 每个轴只动它该动的东西。这里特别验两件容易写错的事：
///   * key_direction 的方位角要环绕到 [-180, 180]；
///   * key_fill_ratio 是**比值**，要按 当前/目标 缩放补光（不是直接赋目标值）。
BOOST_AUTO_TEST_CASE(apply_axis_step_moves_only_the_named_axis) {
  // key_direction：45 + 200 = 245 → 环绕成 -115；补光强度不受影响。
  {
    auto l_rig = build_base_rig(make_model_json());
    const double l_fill_before = l_rig.lights_.at(1).intensity_.value_;
    variant_axis_t l_axis{variant_axis_kind::key_direction, 200.0, "t"};
    BOOST_TEST(apply_axis_step(l_rig, l_axis));
    BOOST_CHECK_CLOSE(l_rig.lights_.at(0).offset_.azimuth_deg_, -115.0, 1e-9);
    BOOST_CHECK_CLOSE(l_rig.lights_.at(1).intensity_.value_, l_fill_before, 1e-9);
  }

  // key_fill_ratio：key=1000、fill=1000 → 当前比值 1；目标 3 → 补光缩放 1/3。
  {
    auto l_rig = build_base_rig(make_model_json());
    variant_axis_t l_axis{variant_axis_kind::key_fill_ratio, 3.0, "t"};
    BOOST_TEST(apply_axis_step(l_rig, l_axis));
    BOOST_CHECK_SMALL(l_rig.lights_.at(1).intensity_.value_ - 1000.0 / 3.0, 1e-9);
    BOOST_CHECK_CLOSE(l_rig.lights_.at(0).intensity_.value_, 1000.0, 1e-9);  // 主光不动

    // 没有补光灯 → 这条轴在当前 rig 上不生效，必须返回 false 而不是假装成功。
    auto l_no_fill = build_base_rig(make_model_json());
    l_no_fill.lights_.erase(l_no_fill.lights_.begin() + 1);
    BOOST_TEST(!apply_axis_step(l_no_fill, l_axis));
  }

  // kelvin：同时作用于灯与全局白平衡。
  {
    auto l_rig = build_base_rig(make_model_json());
    variant_axis_t l_axis{variant_axis_kind::kelvin, -800.0, "t"};
    BOOST_TEST(apply_axis_step(l_rig, l_axis));
    BOOST_CHECK_CLOSE(l_rig.lights_.at(0).color_.kelvin_, 4800.0, 1e-9);
    BOOST_CHECK_CLOSE(l_rig.globals_.white_balance_.kelvin_, 4800.0, 1e-9);
  }

  // grading_strength：只动调色，**不碰灯**（调色与布光两个自由度不能混在一起）。
  {
    auto l_rig = build_base_rig(make_model_json());
    l_rig.globals_.grading_.contrast_ = 1.4;
    const double l_azimuth_before     = l_rig.lights_.at(0).offset_.azimuth_deg_;
    variant_axis_t l_axis{variant_axis_kind::grading_strength, 1.0, "t"};
    BOOST_TEST(apply_axis_step(l_rig, l_axis));
    BOOST_CHECK_CLOSE(l_rig.globals_.grading_.contrast_, 1.8, 1e-9);  // 1 + (1.4-1)*2
    BOOST_CHECK_CLOSE(l_rig.lights_.at(0).offset_.azimuth_deg_, l_azimuth_before, 1e-9);
  }

  // ambient_level：动天光与 role=ambient 的灯。
  {
    auto l_rig = build_base_rig(make_model_json());
    variant_axis_t l_axis{variant_axis_kind::ambient_level, 1.0, "t"};
    BOOST_TEST(apply_axis_step(l_rig, l_axis));
    BOOST_CHECK_CLOSE(l_rig.globals_.environment_.sky_light_intensity_, 2.0, 1e-9);
    BOOST_CHECK_CLOSE(l_rig.lights_.at(2).intensity_.value_, 2000.0, 1e-9);
  }

  // none 永远不是可施加的轴。
  {
    auto l_rig = build_base_rig(make_model_json());
    variant_axis_t l_axis{variant_axis_kind::none, 1.0, "t"};
    BOOST_TEST(!apply_axis_step(l_rig, l_axis));
  }
}

/// OFAT 展开的结构性断言：候选 0 是基线（axis=none），其余每个恰好沿一条轴变化，
/// 而**灯集合与基线逐个相同** —— §6.6 的护栏在这里是构造性事实，不是事后校验。
BOOST_AUTO_TEST_CASE(expand_candidates_is_ofat_around_the_baseline) {
  const auto l_base = build_base_rig(make_model_json());
  const auto l_rigs = expand_candidates(l_base, default_axis_plan(make_spec()));

  BOOST_CHECK_EQUAL(l_rigs.size(), static_cast<std::size_t>(g_candidate_count));
  BOOST_CHECK_EQUAL(std::string{magic_enum::enum_name(l_rigs.at(0).variant_axis_.axis_)}, std::string{"none"});

  const auto l_plan = default_axis_plan(make_spec());
  std::size_t l_expect = 1;
  for (const auto& l_axis : l_plan.axes_) {
    for (const double l_step : l_axis.steps_) {
      const auto& l_rig = l_rigs.at(l_expect);
      BOOST_CHECK_EQUAL(std::string{magic_enum::enum_name(l_rig.variant_axis_.axis_)}, std::string{magic_enum::enum_name(l_axis.axis_)});
      BOOST_CHECK_CLOSE(l_rig.variant_axis_.value_, l_step, 1e-9);
      // §6.6：灯数量与角色集合逐个相同（OFAT 只改变量，不增删灯）。
      BOOST_CHECK_EQUAL(l_rig.lights_.size(), l_base.lights_.size());
      for (std::size_t l_i = 0; l_i < l_rig.lights_.size(); ++l_i) {
        BOOST_CHECK_EQUAL(l_rig.lights_.at(l_i).id_, l_base.lights_.at(l_i).id_);
        BOOST_CHECK_EQUAL(
            std::string{magic_enum::enum_name(l_rig.lights_.at(l_i).role_)},
            std::string{magic_enum::enum_name(l_base.lights_.at(l_i).role_)}
        );
      }
      // ops_ 是**展开后**相对基线算出来的，属于映射阶段的产物；展开器自己不该填。
      BOOST_TEST(l_rig.ops_.empty());
      ++l_expect;
    }
  }
  BOOST_CHECK_EQUAL(l_expect, static_cast<std::size_t>(g_candidate_count));
}

// ---------------------------------------------------------------------------
// diff_rigs：ops 是执行器的唯一输入，静默丢 op == 「轴变了但图没变」
// ---------------------------------------------------------------------------

/// 全局量四块的 diff 曾经**一个 op 都不发**，而且是静默的：`nlohmann::json{globals_}` 花括号构造
/// 得到单元素数组（数组上 contains() 恒 false，见 §15.16 坑 57），键名还写成了带尾下划线的
/// `exposure_`（`globals_t` 的 to_json 键是 `exposure`/`white_balance`/`environment`/`grading`，无尾下划线）。
/// 两个错叠在一起的表现完全一样：「调色改了，但这一轮没有任何全局量 op」。这个用例把
/// 「改一块 → 恰好一个对应 op，patch 是目标整块」钉死。
BOOST_AUTO_TEST_CASE(diff_rigs_emits_one_op_per_changed_global_block) {
  const auto l_base = build_base_rig(make_model_json());

  // 自己跟自己：一个 op 都不发（否则每轮都会下发一堆空改动）。
  BOOST_TEST(diff_rigs(l_base, l_base).empty());

  /// 只动一块 → 恰好一个对应 op；patch 是该块的目标值（set 语义），键名即 to_json 的名字。
  auto l_one_block = [&](auto&& in_mutate, light_op_kind in_kind, const char* in_key) {
    auto       l_tgt = l_base;
    in_mutate(l_tgt);
    const auto l_ops = diff_rigs(l_base, l_tgt);
    BOOST_REQUIRE_EQUAL(l_ops.size(), 1u);
    BOOST_CHECK_EQUAL(en(l_ops.at(0).op_), en(in_kind));
    BOOST_TEST(l_ops.at(0).light_.empty());  // 全局量 op 不绑某一盏灯
    BOOST_TEST(l_ops.at(0).patch_.contains(in_key));
    return l_ops.at(0);
  };

  const auto l_exposure = l_one_block(
      [](lighting_rig_t& io) { io.globals_.exposure_.ev100_bias_ = 0.5; }, light_op_kind::set_exposure, "ev100_bias"
  );
  BOOST_CHECK_CLOSE(l_exposure.patch_.at("ev100_bias").get<double>(), 0.5, 1e-9);

  l_one_block(
      [](lighting_rig_t& io) { io.globals_.white_balance_.kelvin_ = 4200.0; }, light_op_kind::set_white_balance, "kelvin"
  );
  l_one_block(
      [](lighting_rig_t& io) { io.globals_.environment_.fog_density_ = 0.3; }, light_op_kind::set_environment,
      "fog_density"
  );
  const auto l_grading =
      l_one_block([](lighting_rig_t& io) { io.globals_.grading_.contrast_ = 1.4; }, light_op_kind::set_grading, "contrast");
  BOOST_CHECK_CLOSE(l_grading.patch_.at("contrast").get<double>(), 1.4, 1e-9);
  // patch 只装那一块：把整份 globals 塞进去会让执行器「顺手」改别的块。
  BOOST_TEST(!l_grading.patch_.contains("sky_light_intensity"));

  // 两块一起动 → 两个 op，顺序与实现里的表一致（exposure → white_balance → environment → grading）。
  {
    auto l_tgt                           = l_base;
    l_tgt.globals_.grading_.vignette_    = 0.25;
    l_tgt.globals_.exposure_.ev100_bias_ = -0.5;
    const auto l_ops                     = diff_rigs(l_base, l_tgt);
    BOOST_REQUIRE_EQUAL(l_ops.size(), 2u);
    BOOST_CHECK_EQUAL(en(l_ops.at(0).op_), en(light_op_kind::set_exposure));
    BOOST_CHECK_EQUAL(en(l_ops.at(1).op_), en(light_op_kind::set_grading));
  }

  // 逐灯：patch 只装真的变了的子块，并绑上那盏灯的语义 id（执行器按 id 定位，不按下标）。
  {
    auto l_tgt                           = l_base;
    l_tgt.lights_.at(1).intensity_.value_ = 750.0;
    const auto l_ops                      = diff_rigs(l_base, l_tgt);
    BOOST_REQUIRE_EQUAL(l_ops.size(), 1u);
    BOOST_CHECK_EQUAL(en(l_ops.at(0).op_), en(light_op_kind::modify));
    BOOST_CHECK_EQUAL(l_ops.at(0).light_, l_base.lights_.at(1).id_);
    BOOST_TEST(l_ops.at(0).patch_.contains("intensity"));
    BOOST_TEST(!l_ops.at(0).patch_.contains("offset"));  // 没变的子块不出现
  }
}

// ---------------------------------------------------------------------------
// 映射：钉死字段、写 ops、拒绝静默失败
// ---------------------------------------------------------------------------

/// 服务端口径的字段一律不取模型的值；每个变异候选都要带上相对基线的 ops。
BOOST_AUTO_TEST_CASE(map_result_pins_identity_and_emits_ops) {
  auto l_req       = make_request();
  l_req.iteration_ = 3;

  const auto l_res = map_candidate_result(make_model_json(), l_req);

  BOOST_CHECK_EQUAL(l_res.rigs_.size(), static_cast<std::size_t>(g_candidate_count));
  BOOST_CHECK_EQUAL(l_res.axes_.axes_.size(), 2u);
  BOOST_TEST(!l_res.axes_.strategy_.empty());

  for (std::size_t l_i = 0; l_i < l_res.rigs_.size(); ++l_i) {
    const auto& l_rig = l_res.rigs_.at(l_i);
    BOOST_CHECK_EQUAL(l_rig.session_id_, l_req.session_id_);
    BOOST_CHECK_EQUAL(l_rig.spec_id_, l_req.spec_.spec_id_);
    BOOST_CHECK_EQUAL(l_rig.iteration_, 3);
    BOOST_CHECK_EQUAL(l_rig.candidate_index_, static_cast<std::int32_t>(l_i));
    BOOST_CHECK_EQUAL(l_rig.baseline_level_, l_req.baseline_level_);
    BOOST_TEST(l_rig.rig_id_.is_nil());  // 由调用方一次性分配，接缝不落库也不需要它唯一
    BOOST_CHECK_EQUAL(l_rig.provenance_.model_, doodle_config::g_ark_inference_model);
    BOOST_TEST(!l_rig.provenance_.prompt_hash_.empty());

    if (l_i == 0) {
      BOOST_TEST(l_rig.ops_.empty());  // 基线 = 被比较的对象，相对自己没有 op
      continue;
    }
    BOOST_TEST(!l_rig.ops_.empty());
    // §6.6：OFAT 只改变量，ops 里**不允许**出现增删灯。
    for (const auto& l_op : l_rig.ops_) {
      BOOST_TEST(en(l_op.op_) != en(light_op_kind::add));
      BOOST_TEST(en(l_op.op_) != en(light_op_kind::remove));
    }
  }
}

/// prompt_hash_ 是审计指纹：同一份输入必须得到同一个值（§8.5 第 1 条：可复现）。
/// 注意 spec_id_ 参与身份、但不参与 input_text，所以这里保持同一个 request。
BOOST_AUTO_TEST_CASE(map_result_prompt_hash_is_deterministic) {
  const auto l_req = make_request();
  const auto l_a   = map_candidate_result(make_model_json(), l_req);
  const auto l_b   = map_candidate_result(make_model_json(), l_req);
  BOOST_CHECK_EQUAL(l_a.rigs_.at(1).provenance_.prompt_hash_, l_b.rigs_.at(1).provenance_.prompt_hash_);
}

/// 轴计划不合法**不抛错**：它是模型的一种建议，弃用它退回默认计划仍然能给出 6 个可用候选，
/// 比让整轮失败好。弃用原因必须留在 strategy_ 里（否则「为什么这轮是这几个档位」无从追溯）。
BOOST_AUTO_TEST_CASE(map_result_falls_back_on_illegal_axis_plan) {
  // 0 档 + 档位总数不足，两个问题都违规。
  auto l_json          = make_model_json();
  l_json["axes"]       = nlohmann::json::array({nlohmann::json{
      {"axis", "key_fill_ratio"},
      {"steps", nlohmann::json::array({3.0, 0.0, 6.0})},
      {"label", "x"}
  }});
  const auto l_req     = make_request();
  const auto l_res     = map_candidate_result(l_json, l_req);
  const auto l_default = default_axis_plan(l_req.spec_);
  BOOST_CHECK_EQUAL(l_res.rigs_.size(), static_cast<std::size_t>(g_candidate_count));
  BOOST_TEST(l_res.axes_.strategy_.find("弃用") != std::string::npos);
  // 退回的必须是**默认轴计划本身**：不是空轴表，也不是残留的半截模型计划。
  BOOST_CHECK_EQUAL(l_res.axes_.axes_.size(), l_default.axes_.size());
  for (std::size_t l_i = 0; l_i < l_default.axes_.size(); ++l_i) {
    BOOST_CHECK_EQUAL(en(l_res.axes_.axes_.at(l_i).axis_), en(l_default.axes_.at(l_i).axis_));
    BOOST_CHECK_EQUAL(l_res.axes_.axes_.at(l_i).steps_.size(), l_default.axes_.at(l_i).steps_.size());
  }

  // 未知轴名同样退回默认计划，而不是整轮失败。
  auto l_bad_axis          = make_model_json();
  l_bad_axis["axes"]       = nlohmann::json::array({nlohmann::json{
      {"axis", "key_elevation"},
      {"steps", nlohmann::json::array({-15.0, 15.0, 30.0, 45.0, 60.0})},
      {"label", "x"}
  }});
  const auto l_res2 = map_candidate_result(l_bad_axis, make_request());
  BOOST_CHECK_EQUAL(l_res2.rigs_.size(), static_cast<std::size_t>(g_candidate_count));
  BOOST_TEST(l_res2.axes_.strategy_.find("弃用") != std::string::npos);
}

/// §9.1 的灵敏度前提：一条轴在展开后什么都没改，就会产生一张与基线**一模一样**的候选图。
/// 返回 6 张看起来一样的图比一次失败更糟 —— 那会让美术以为「这几个方向看起来一样」。
BOOST_AUTO_TEST_CASE(map_result_rejects_axis_without_effect) {
  auto l_json    = make_model_json();
  l_json["lights"] = nlohmann::json::array({make_light_json("key", 45.0), make_light_json("ambient", 0.0)});
  l_json["axes"]   = nlohmann::json::array({nlohmann::json{
      {"axis", "key_fill_ratio"},
      {"steps", nlohmann::json::array({3.0, 6.0, 10.0, 2.0, 20.0})},
      {"label", "x"}
  }});
  BOOST_CHECK_THROW(map_candidate_result(l_json, make_request()), doodle_error);
}

/// 没有主光就等于 key_direction / key_fill_ratio 两个轴都是空操作 —— 这是配置错误，
/// 不是「这一轮不巧」，所以当场 502 而不是悄悄给出一堆无效候选。
BOOST_AUTO_TEST_CASE(map_result_rejects_missing_key_light) {
  auto l_json      = make_model_json();
  l_json["lights"] = nlohmann::json::array({make_light_json("fill", -30.0), make_light_json("ambient", 0.0)});
  BOOST_CHECK_THROW(map_candidate_result(l_json, make_request()), doodle_error);
}

/// 非 object（含 null、数组）一律 502。**不**二次解析 result_text_ ——
/// parse_inference_response 内部已经做过 strip_code_fence + parse 了。
BOOST_AUTO_TEST_CASE(map_result_rejects_non_object) {
  const auto l_req = make_request();
  BOOST_CHECK_THROW(map_candidate_result(nlohmann::json::array(), l_req), doodle_error);
  BOOST_CHECK_THROW(map_candidate_result(nlohmann::json{}, l_req), doodle_error);   // {} 是 object 但不是合法 rig
  BOOST_CHECK_THROW(map_candidate_result(nlohmann::json{nullptr}, l_req), doodle_error);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace doodle::ai_light
