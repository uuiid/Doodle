//
// AI 灯光：接缝 3 的离线可测部件（docs/ue_ai_lighting_plan.md §15.9）
//
// structure_feedback 本体要联网（火山方舟 Responses）且需要 SSL context，单测宿主两样都没有，
// 所以这里盯的是**离线那一半** —— 组装推理信封 + 把模型给的「轴 + 一个方向值」映射成 axis_step。
//
// 这份测试钉的是三件事：
//   1) 接缝 3 与接缝 2 共用**同一份**轴值语义，但不继承接缝 2 的档位规则
//      （档位规则套在一个方向值上会自相矛盾）；
//   2) 语义无效的轴（none / 0 / 未知轴名 / 非正 key:fill 比）在**源头**丢弃 ——
//      它们一旦流到下一轮，apply_axis_step() 恒返回 false，最终以「轴无实质变化」在
//      下一轮 /iterate 抛 502，而那时病根与表现之间已经隔了一次人工判断；
//   3) 提示词里给模型的数字必须是**算得出来**的数字：key:fill 比算不出来时要明说
//      「不要给这条轴」，而不是给一个假的比值。
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
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
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
  l_person.subject_                           = "主角";
  l_person.trait_                             = "阴郁内向";
  l_person.lighting_language_.hardness_       = "hard";
  l_person.lighting_language_.position_bias_  = "side_high";
  l_person.lighting_language_.kelvin_bias_    = "cool";
  l_person.lighting_language_.key_fill_ratio_ = interval_t{6.0, 10.0};
  l_spec.intent_.character_personality_       = std::vector<character_personality_t>{l_person};

  l_spec.targets_.contrast_                                    = interval_t{0.62, 0.72};
  l_spec.targets_.white_balance_kelvin_                        = interval_t{4000.0, 4200.0};
  l_spec.reference_stats_.per_dimension_confidence_["contrast"] = 0.8;
  return l_spec;
}

/// 一份「模型合法输出」的最小样本：一盏主光 + 一盏补光 + 一盏环境灯。
/// 主光与补光强度相同且同单位，于是基线 key:fill 比恰好是 1。
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

nlohmann::json make_model_json() {
  return nlohmann::json{
      {"strategy", "首轮大跨度探索对比结构。"},
      {"axes",
       nlohmann::json::array({
           nlohmann::json{
               {"axis", "key_fill_ratio"},
               {"steps", nlohmann::json::array({3.0, 6.0, 10.0})},
               {"label", "对比档位"}
           },
           nlohmann::json{{"axis", "kelvin"}, {"steps", nlohmann::json::array({-800.0, 800.0})}, {"label", "冷暖"}},
       })},
      {"globals", make_globals_json()},
      {"lights",
       nlohmann::json::array({
           make_light_json("key", 45.0),
           make_light_json("fill", -30.0),
           make_light_json("ambient", 0.0),
       })},
  };
}

/// 6 个候选：直接复用接缝 2 的确定性展开器（build_base_rig + expand_candidates），
/// 于是这里的候选与生产路径里落库的候选**在结构上完全同形** —— 包括每套 rig 的
/// variant_axis_（接缝 3 的提示词正是从候选 1..5 的 variant_axis_ 读轴与档位的）。
std::vector<ai_light_candidate> make_candidates() {
  iteration_axes_t l_axes{};
  l_axes.axes_ = std::vector<axis_step_t>{
      axis_step_t{variant_axis_kind::key_fill_ratio, {3.0, 6.0, 10.0}, "对比档位"},
      axis_step_t{variant_axis_kind::kelvin, {-800.0, 800.0}, "冷暖"},
  };
  const auto l_rigs = expand_candidates(build_base_rig(make_model_json()), l_axes);

  std::vector<ai_light_candidate> l_out{};
  for (std::size_t l_i = 0; l_i < l_rigs.size(); ++l_i) {
    ai_light_candidate l_cand{};
    l_cand.uuid_id_         = core_set::get_set().get_uuid();
    l_cand.candidate_index_ = static_cast<std::int32_t>(l_i);
    l_cand.set_rig(l_rigs.at(l_i));
    l_out.emplace_back(std::move(l_cand));
  }
  return l_out;
}

feedback_structuring_request_t make_request() {
  feedback_structuring_request_t l_req{};
  l_req.session_id_    = core_set::get_set().get_uuid();
  l_req.project_id_    = core_set::get_set().get_uuid();
  l_req.operator_id_   = core_set::get_set().get_uuid();
  l_req.iteration_id_  = core_set::get_set().get_uuid();
  l_req.iteration_     = 2;
  l_req.feedback_text_ = "暖一点、柔一点";
  l_req.decision_      = judgement_decision::continue_iteration;
  l_req.spec_          = make_spec();
  l_req.candidates_    = make_candidates();
  l_req.selected_candidate_id_ = l_req.candidates_.at(3).uuid_id_;
  return l_req;
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

/// 一行轴，省掉每次写三遍键名。
nlohmann::json make_axis_line(const char* in_axis, double in_value, const char* in_reason) {
  return nlohmann::json{{"axis", in_axis}, {"value", in_value}, {"reason", in_reason}};
}

}  // namespace

BOOST_AUTO_TEST_SUITE(ai_light_feedback)

// ---------------------------------------------------------------------------
// schema 与提示词
// ---------------------------------------------------------------------------

/// 轴枚举必须与 C++ 枚举逐字一致且**不含 none**（none 是「这是基线」的标记，不是方向），
/// 并且**不设 minItems** —— 空 axes 是「美术没给出方向」的合法表达（决定 D13），
/// 加了 minItems 等于逼模型编一条不存在的方向出来。
BOOST_AUTO_TEST_CASE(axis_enum_excludes_none_and_schema_is_strict) {
  const auto l_schema = build_feedback_json_schema();

  BOOST_TEST(l_schema.at("type").get<std::string>() == "object");
  BOOST_TEST(l_schema.at("additionalProperties").get<bool>() == false);

  // 每层 required 必须覆盖全部 properties：漏一个键，方舟的 strict 模式会直接拒掉整次请求。
  // 注意先绑成 const auto 再取 begin()/end() —— keys_of()/to_strings() 按值返回 vector，
  // 一次表达式里两次调用会取到**两个不同临时对象**的迭代器，MSVC 调试迭代器当场 abort
  // （vector iterators incompatible，§15.8 坑 22）。
  const auto l_top_keys = keys_of(l_schema.at("properties"));
  const auto l_top_req  = to_strings(l_schema.at("required"));
  BOOST_CHECK_EQUAL_COLLECTIONS(l_top_keys.begin(), l_top_keys.end(), l_top_req.begin(), l_top_req.end());

  const auto& l_axes = l_schema.at("properties").at("axes");
  BOOST_TEST(l_axes.at("type").get<std::string>() == "array");
  BOOST_TEST(l_axes.at("maxItems").get<std::size_t>() == 2u);
  BOOST_TEST(!l_axes.contains("minItems"));

  const auto& l_item = l_axes.at("items");
  BOOST_TEST(l_item.at("additionalProperties").get<bool>() == false);
  const auto l_item_keys = keys_of(l_item.at("properties"));
  const auto l_item_req  = to_strings(l_item.at("required"));
  BOOST_CHECK_EQUAL_COLLECTIONS(l_item_keys.begin(), l_item_keys.end(), l_item_req.begin(), l_item_req.end());

  // 枚举名对不上的代价是模型给一个名字、服务端 enum_cast 失败 → 整次判断的文本通道静默失效。
  const auto l_enum = to_strings(l_item.at("properties").at("axis").at("enum"));
  BOOST_TEST(l_enum.size() == magic_enum::enum_count<variant_axis_kind>() - 1);
  // 注意：断言里直接比较迭代器（`std::find(...) != x.end()`）会让 Boost.Test 去打印**迭代器**，
  // 而迭代器没有 operator<< → print_helper 的 static_assert 直接把编译打断（承 §15.8 坑 22 一类）。
  // 先收敛成 bool 再断言。
  const bool l_has_none = std::find(l_enum.begin(), l_enum.end(), std::string{"none"}) != l_enum.end();
  const bool l_has_kfr  = std::find(l_enum.begin(), l_enum.end(), std::string{"key_fill_ratio"}) != l_enum.end();
  BOOST_TEST(!l_has_none);
  BOOST_TEST(l_has_kfr);

  // 服务端钉死的身份字段一个都不许出现在 schema 里（模型可写面 = schema 的面）。
  const std::string l_dump = l_schema.dump();
  for (const std::string& l_forbidden : {"session_id", "spec_id", "rig_id", "schema_version", "candidate_index"})
    BOOST_TEST(l_dump.find(l_forbidden) == std::string::npos);
}

/// 接缝 3 与接缝 2 共用轴值语义，但**不能**继承接缝 2 的档位规则：
/// 接缝 2 要 5 个档位、接缝 3 每条轴只给一个方向值，两套规则放一起是自相矛盾的指令。
BOOST_AUTO_TEST_CASE(instructions_split_the_shared_semantics_from_seam2_constraints) {
  const std::string l_fb   = build_feedback_instructions();
  const std::string l_cand = build_candidates_instructions();

  // 两边都必须带同一份轴值语义（口径唯一：模型按它给，服务端按同一套公式应用）。
  BOOST_TEST(l_fb.find("增量") != std::string::npos);
  BOOST_TEST(l_cand.find("增量") != std::string::npos);
  BOOST_TEST(l_fb.find("key_fill_ratio") != std::string::npos);

  // 排轴的档位规则必须留在接缝 2。
  BOOST_TEST(l_fb.find("恰好 5 个档位") == std::string::npos);
  BOOST_TEST(l_fb.find("档位不要取 0") == std::string::npos);
  BOOST_TEST(l_fb.find("最多声明 2 个轴") == std::string::npos);
  BOOST_TEST(l_cand.find("恰好 5 个档位") != std::string::npos);

  // 接缝 3 自己的上限必须写进提示词：下一轮 parse_axis_plan() 只接受 2 条轴，
  // 而 feedback_axes 是**约束** —— 给 3 条会让整份轴计划被弃用、文本通道静默失效（决定 D12）。
  BOOST_TEST(l_fb.find("最多给 2 条轴") != std::string::npos);
  // 空数组这条出口也要明确允许，否则模型会为了凑数编一个方向。
  BOOST_TEST(l_fb.find("空数组") != std::string::npos);
}

// ---------------------------------------------------------------------------
// 输入提示词
// ---------------------------------------------------------------------------

/// 美术的原话必须**逐字**进提示词，选中候选要按界面上的序号（#3）指出来 ——
/// 原话里的「第 3 个」指的就是界面排序，模型与美术必须看到同一个编号。
BOOST_AUTO_TEST_CASE(input_text_carries_verbatim_feedback_and_candidates) {
  auto l_req              = make_request();
  l_req.feedback_text_    = "第 3 个太冷了，暖一点，主光再低一些。";
  l_req.selected_candidate_id_ = l_req.candidates_.at(3).uuid_id_;

  const std::string l_text = build_feedback_input_text(l_req);

  BOOST_TEST(l_text.find("第 3 个太冷了，暖一点，主光再低一些。") != std::string::npos);
  BOOST_TEST(l_text.find("美术选中了候选 #3") != std::string::npos);
  BOOST_TEST(l_text.find("决定 = continue_iteration") != std::string::npos);

  // Spec 目标（同一个投影函数，接缝 1/2/3 看到的 Spec 表述必须一致）。
  BOOST_TEST(l_text.find("雨夜街道") != std::string::npos);
  BOOST_TEST(l_text.find("阴郁内向") != std::string::npos);

  // 6 个候选的对照结构：基线整套 + 其余只列轴/值/标签（OFAT 让这足够描述 6 张图）。
  BOOST_TEST(l_text.find("#0 基线：") != std::string::npos);
  BOOST_TEST(l_text.find("#1  轴=key_fill_ratio  值=3  对比档位") != std::string::npos);
  BOOST_TEST(l_text.find("#2  轴=key_fill_ratio  值=6  对比档位") != std::string::npos);
  BOOST_TEST(l_text.find("#4  轴=kelvin  值=-800  冷暖") != std::string::npos);
  // 选中候选的完整 rig 也要给（原话最可能指的就是它）。
  BOOST_TEST(l_text.find("【美术选中的那套") != std::string::npos);
  // 绝对量轴必须知道现状。
  BOOST_TEST(l_text.find("【当前 key:fill 比】") != std::string::npos);
  BOOST_TEST(l_text.find("1（基线 rig 的主光强度") != std::string::npos);
}

/// 滑块已经给出的轴要明确标成「不要再给」：同一轴上滑块优先（决定 D9），
/// 不说清的话模型会在这里给一条必然被覆盖的轴，白费一次输出。
BOOST_AUTO_TEST_CASE(input_text_marks_slider_axes) {
  auto l_req              = make_request();
  l_req.slider_axes_      = {{"kelvin", -400.0}};
  const std::string l_text = build_feedback_input_text(l_req);

  BOOST_TEST(l_text.find("【美术已经用滑块给出的方向】") != std::string::npos);
  BOOST_TEST(l_text.find("kelvin = -400") != std::string::npos);
  BOOST_TEST(l_text.find("不要再给") != std::string::npos);

  l_req.slider_axes_.clear();
  BOOST_TEST(build_feedback_input_text(l_req).find("没有（美术只写了字）。") != std::string::npos);
}

/// 候选还没落库、或美术没指定候选，都不能让提示词崩掉或说错话：
/// 这两种情况在本轮是**正常**的（候选渲染失败 / 美术只说「换一批」）。
BOOST_AUTO_TEST_CASE(input_text_survives_empty_candidates_and_no_selection) {
  auto l_req                    = make_request();
  l_req.candidates_.clear();
  l_req.selected_candidate_id_  = uuid{};

  const std::string l_text = build_feedback_input_text(l_req);
  BOOST_TEST(l_text.find("本轮候选还没落库") != std::string::npos);
  BOOST_TEST(l_text.find("美术没有指定候选") != std::string::npos);
  BOOST_TEST(l_text.find("#0 基线：") == std::string::npos);
  // 没有候选就没有 key:fill 比可算 —— 宁可不给这一段，也不给一个假数字。
  BOOST_TEST(l_text.find("【当前 key:fill 比】") == std::string::npos);

  // 选中 id 指向一个不在这轮列表里的候选（例如上一轮的候选）：要说清，而不是静默当成没选。
  l_req.selected_candidate_id_ = core_set::get_set().get_uuid();
  BOOST_TEST(build_feedback_input_text(l_req).find("不在这轮的候选列表里") != std::string::npos);
}

// ---------------------------------------------------------------------------
// key:fill 比
// ---------------------------------------------------------------------------

/// key:fill 比是**绝对量**轴的基准值，算错方向就全错。所以单位不同时必须明说算不出来，
/// 而不是给一个把 candela 和 lumen 加在一起的数字。
BOOST_AUTO_TEST_CASE(key_fill_ratio_of_is_unit_aware) {
  // 1) 主光 1000 + 补光 1000（同 candela）→ 1.0
  auto l_rig           = build_base_rig(make_model_json());
  const auto l_ratio   = key_fill_ratio_of(l_rig);
  BOOST_TEST(l_ratio.has_value());
  BOOST_TEST(*l_ratio == 1.0);

  // 2) 补光换成别的量纲 → 算不出来（不是「大致等于 1」）
  auto l_other_unit = build_base_rig(make_model_json());
  for (auto& l_light : l_other_unit.lights_)
    if (l_light.role_ == light_role::fill) l_light.intensity_.unit_ = intensity_unit::lumen;
  BOOST_TEST(!key_fill_ratio_of(l_other_unit).has_value());

  // 3) 没有补光 → 除数 0，不许返回 inf
  auto l_no_fill = build_base_rig(make_model_json());
  l_no_fill.lights_.erase(
      std::remove_if(
          l_no_fill.lights_.begin(), l_no_fill.lights_.end(),
          [](const light_t& in_light) { return in_light.role_ == light_role::fill; }
      ),
      l_no_fill.lights_.end()
  );
  BOOST_TEST(!key_fill_ratio_of(l_no_fill).has_value());

  // 4) 主光强度为 0 → 比值没有意义
  auto l_zero_key = build_base_rig(make_model_json());
  for (auto& l_light : l_zero_key.lights_)
    if (l_light.role_ == light_role::key) l_light.intensity_.value_ = 0.0;
  BOOST_TEST(!key_fill_ratio_of(l_zero_key).has_value());

  // 5) 两盏补光按**和**算：1000 ÷ (1000 + 1000) = 0.5
  auto l_two_fill = build_base_rig(make_model_json());
  light_t l_fill_copy{};
  bool l_found_fill = false;
  for (const auto& l_light : l_two_fill.lights_)
    if (l_light.role_ == light_role::fill) {
      l_fill_copy  = l_light;
      l_found_fill = true;
    }
  BOOST_TEST(l_found_fill);
  l_two_fill.lights_.push_back(l_fill_copy);
  const auto l_two = key_fill_ratio_of(l_two_fill);
  BOOST_TEST(l_two.has_value());
  BOOST_TEST(*l_two == 0.5);
}

// ---------------------------------------------------------------------------
// 映射
// ---------------------------------------------------------------------------

/// 每条轴**恰好一个值**（决定 D11）：调用方只取 front() 当下一轮的中心值，
/// 档位表由下一轮的排轴环节决定。给多个值不是「更详细」，而是给调用方一个会被静默丢掉的东西。
BOOST_AUTO_TEST_CASE(map_result_returns_one_step_per_axis) {
  const nlohmann::json l_json{
      {"understanding", "美术觉得太冷，要暖一点、主光低一些"},
      {"axes",
       nlohmann::json::array({
           make_axis_line("kelvin", 400.0, "冷 → 暖"),
           make_axis_line("key_direction", -30.0, "主光低一些"),
       })},
  };

  const auto l_steps = map_feedback_result(l_json);
  BOOST_TEST(l_steps.size() == 2u);
  BOOST_TEST(en(l_steps.at(0).axis_) == "kelvin");
  BOOST_TEST(l_steps.at(0).steps_.size() == 1u);
  BOOST_TEST(l_steps.at(0).steps_.front() == 400.0);
  BOOST_TEST(l_steps.at(0).label_ == "冷 → 暖");
  BOOST_TEST(en(l_steps.at(1).axis_) == "key_direction");
  BOOST_TEST(l_steps.at(1).steps_.front() == -30.0);

  // reason 缺失不是形状违规：label_ 回落成轴名（label_ 会回显给美术，不能是空串）。
  const nlohmann::json l_no_reason{
      {"understanding", "x"},
      {"axes", nlohmann::json::array({nlohmann::json{{"axis", "hardness"}, {"value", 0.5}}})},
  };
  const auto l_one = map_feedback_result(l_no_reason);
  BOOST_TEST(l_one.size() == 1u);
  BOOST_TEST(l_one.at(0).label_ == "hardness");
}

/// 语义无效的轴在**源头**丢弃：none / 0 是「没有变化」，未知轴名是模型自由发挥，
/// 非正的 key:fill 比在 apply_axis_step() 里也直接返回 false。
/// 它们流到下一轮的表现是「下一轮 /iterate 抛 502」，而那时美术已经白判了一轮。
/// 丢弃与「未知轴名忽略」的既有容错（advance_session）一致；形状违规才抛错。
BOOST_AUTO_TEST_CASE(map_result_drops_none_zero_unknown_and_bad_ratio) {
  // 注意：`axes` 的 maxItems 校验发生在**语义过滤之前**（形状违规先于语义丢弃），
  // 所以「这些轴一条都不该留下」不能堆成一个 6 条的数组 —— 那样先撞上的会是「超过 2 条」。
  // 分批喂，每批 ≤ 2 条。
  const auto l_drop = [](const nlohmann::json& in_axes) {
    nlohmann::json l_json{};
    l_json["understanding"] = "混合了合法与无效的轴";
    l_json["axes"]            = in_axes;
    return map_feedback_result(l_json);
  };

  // 1) none：是合法枚举值（enum_cast 会接受），但没有方向语义
  BOOST_TEST(l_drop(nlohmann::json::array({make_axis_line("none", 1.0, "这不是方向")})).empty());
  // 2) value == 0：等价于基线，模型说「不变」不该占掉一个轴
  BOOST_TEST(l_drop(nlohmann::json::array({make_axis_line("kelvin", 0.0, "0 等于没有变化")})).empty());
  // 3) 未知轴名：模型自由发挥
  BOOST_TEST(l_drop(nlohmann::json::array({make_axis_line("key_elevation", -20.0, "轴名不存在")})).empty());
  // 4) key_fill_ratio 非正：apply_axis_step() 对 <= 0 也直接返回 false
  BOOST_TEST(l_drop(nlohmann::json::array({make_axis_line("key_fill_ratio", -2.0, "比值不能是负的")})).empty());
  BOOST_TEST(l_drop(nlohmann::json::array({make_axis_line("key_fill_ratio", 0.0, "比值不能是 0")})).empty());

  // 5) 同一批里合法的留下、无效的丢掉 —— 丢的是那一条，不是整份结果
  const auto l_mixed = l_drop(nlohmann::json::array({
      make_axis_line("none", 1.0, "这不是方向"),
      make_axis_line("ambient_level", 0.4, "抬环境光"),
  }));
  BOOST_TEST(l_mixed.size() == 1u);
  BOOST_TEST(en(l_mixed.at(0).axis_) == "ambient_level");
  BOOST_TEST(l_mixed.at(0).steps_.front() == 0.4);
}

/// 同一条轴出现两次时取**第一个**：保持确定性，别让「最后一个赢」取决于模型输出顺序。
BOOST_AUTO_TEST_CASE(map_result_keeps_first_of_duplicate_axis) {
  const nlohmann::json l_json{
      {"understanding", "同一条轴说了两遍"},
      {"axes",
       nlohmann::json::array({
           make_axis_line("kelvin", -600.0, "第一次说冷"),
           make_axis_line("kelvin", 900.0, "第二次说暖"),
       })},
  };

  const auto l_steps = map_feedback_result(l_json);
  BOOST_TEST(l_steps.size() == 1u);
  BOOST_TEST(l_steps.at(0).steps_.front() == -600.0);
  BOOST_TEST(l_steps.at(0).label_ == "第一次说冷");
}

/// 空 axes 是合法输入（决定 D13）：「再调调看」「换一批」这类没有方向的反馈不该逼模型编一条。
/// 返回空 vector 后调用方退回纯滑块通道，判断照样成立。
BOOST_AUTO_TEST_CASE(map_result_accepts_empty_axes) {
  const nlohmann::json l_json{
      {"understanding", "美术只是说再调调看"},
      {"axes", nlohmann::json::array()},
  };
  BOOST_TEST(map_feedback_result(l_json).empty());
}

/// 形状被无视才抛错：这些情况说明模型没按 schema 输出，静默接受等于把一次协议违规
/// 变成一条看起来正常的轴。
BOOST_AUTO_TEST_CASE(map_result_rejects_shape_violations) {
  const nlohmann::json l_null = nullptr;
  BOOST_CHECK_THROW(map_feedback_result(l_null), doodle_error);

  // 缺 understanding
  BOOST_CHECK_THROW(
      map_feedback_result(nlohmann::json{{"axes", nlohmann::json::array()}}), doodle_error
  );
  // understanding 不是字符串
  BOOST_CHECK_THROW(
      map_feedback_result(nlohmann::json{{"understanding", 1}, {"axes", nlohmann::json::array()}}), doodle_error
  );
  // 缺 axes
  BOOST_CHECK_THROW(map_feedback_result(nlohmann::json{{"understanding", "x"}}), doodle_error);
  // axes 不是数组
  BOOST_CHECK_THROW(
      map_feedback_result(nlohmann::json{{"understanding", "x"}, {"axes", "kelvin"}}), doodle_error
  );
  // axes 超过 2 条：下一轮的 parse_axis_plan() 只接受 2 条，多给的会让整份轴计划被弃用
  BOOST_CHECK_THROW(
      map_feedback_result(
          nlohmann::json{
              {"understanding", "x"},
              {"axes",
               nlohmann::json::array({
                   make_axis_line("kelvin", 300.0, "a"),
                   make_axis_line("hardness", 0.3, "b"),
                   make_axis_line("ambient_level", 0.2, "c"),
               })}
          }
      ),
      doodle_error
  );
  // 元素不是 object
  BOOST_CHECK_THROW(
      map_feedback_result(nlohmann::json{{"understanding", "x"}, {"axes", nlohmann::json::array({"kelvin"})}}),
      doodle_error
  );
  // 元素缺 axis
  BOOST_CHECK_THROW(
      map_feedback_result(
          nlohmann::json{{"understanding", "x"}, {"axes", nlohmann::json::array({nlohmann::json{{"value", 1.0}}})}}
      ),
      doodle_error
  );
  // value 不是数字
  BOOST_CHECK_THROW(
      map_feedback_result(
          nlohmann::json{
              {"understanding", "x"},
              {"axes", nlohmann::json::array({nlohmann::json{{"axis", "kelvin"}, {"value", "暖"}}})}
          }
      ),
      doodle_error
  );
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace doodle::ai_light
