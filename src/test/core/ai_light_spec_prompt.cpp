//
// AI 灯光：接缝 1 的离线可测部件（docs/ue_ai_lighting_plan.md §15.7）
//
// generate_spec 本体要联网（方舟 Responses）且需要 SSL context，单测宿主两样都没有，
// 所以这里盯的是**离线那一半** —— 组装推理信封 + 把模型输出映射回 LightingSpec。
// 这半决定了「服务端究竟把什么交给模型、又把模型的什么当作事实」，是能离线钉死的部分。
//
// 这份测试的两个立场（与实现里的注释一致）：
//   * 服务端口径的东西（调色板、ownership、space、schema_version、reference_stats）
//     必须**不因模型输出而改变** —— 模型给什么都不该动；
//   * 模型的区间值一旦与统计不同，就按 §6.3「提示词优先于参考」采用模型的值。
//     （原先还会把它记进 conflict_resolution 的 overridden_dimensions 供人工闸门 1 核对，
//     该字段已删除；现在这条规则只体现在最终取值上。）
//

// 刻意**不**包含路由头 http_method/kitsu/ai_light.h：它 include 了
// <core/http/http_function.h>（相对 doodle_lib 根的写法），而测试目标的 include 路径里
// 没有 E:\Doodle\src\doodle_lib。
// 所以这里不引用 ai_light.h 的 reference_image_url，改成在下面按线格式自己拼期望值 ——
// 这反而更好：测试独立钉住 URL 形状，而不是跟着实现一起改。
#include <doodle_lib/core/core_set.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_light/ai_light.h>

#include <doodle_lib/http_method/kitsu/ai_light_orchestrator.h>
#include <doodle_lib/http_method/kitsu/ai_light_stats.h>
// 消费端（build_inference_body）：这里要钉的是「生产端信封 → 方舟 body」这条链，
// 所以测试必须能拿到翻译器本体。它只依赖 nlohmann/json + 少量 HTTP 客户端基础设施，
// 测试目标已经在 seedance2_inference.cpp 里 include 过（同一二进制），没有新增依赖。
#include <doodle_lib/http_client/seedance2_client.h>

#include <doodle_core/configure/static_value.h>

#include <boost/test/unit_test.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

#include <algorithm>
#include <string>
#include <vector>

namespace doodle::ai_light {
namespace {

/// 一份「有可用统计」的聚合结果：两个维度给了置信度 → 会被判成 stats_only。
aggregated_reference_stats_t make_agg(bool in_with_stats = true) {
  aggregated_reference_stats_t l_agg{};
  l_agg.stats_.image_count_      = 3;
  l_agg.stats_.aggregation_      = "median";
  l_agg.stats_.retouch_detected_ = false;
  if (in_with_stats) {
    l_agg.stats_.per_dimension_confidence_["contrast"]             = 0.8;
    l_agg.stats_.per_dimension_confidence_["white_balance_kelvin"] = 0.6;
  }
  l_agg.targets_.space_                = "display_srgb";
  l_agg.targets_.contrast_             = interval_t{0.55, 0.75};
  l_agg.targets_.white_balance_kelvin_ = interval_t{4000.0, 4200.0};
  l_agg.targets_.luma_percentiles_.p10_ = interval_t{0.05, 0.08};
  l_agg.targets_.luma_percentiles_.p50_ = interval_t{0.40, 0.46};
  l_agg.targets_.luma_percentiles_.p90_ = interval_t{0.72, 0.80};
  l_agg.targets_.clip_rate_.high_       = interval_t{0.0, 0.01};
  l_agg.targets_.clip_rate_.low_        = interval_t{0.0, 0.02};
  l_agg.targets_.grading_hints_.vignette_      = "subtle";
  l_agg.targets_.grading_hints_.lifted_blacks_ = false;
  l_agg.targets_.grading_hints_.bloom_         = "none";
  l_agg.targets_.palette_ = std::vector<palette_entry_t>{
      palette_entry_t{vec3_t{0.10, 0.12, 0.20}, interval_t{0.40, 0.60}},
      palette_entry_t{vec3_t{0.90, 0.70, 0.40}, interval_t{0.10, 0.25}},
  };
  l_agg.targets_.ownership_.lighting_ = std::vector<std::string>{"intensity", "direction"};
  l_agg.targets_.ownership_.grading_  = std::vector<std::string>{"contrast", "vignette"};
  l_agg.targets_.ownership_.shared_   = std::vector<std::string>{"white_balance"};
  return l_agg;
}

ai_light_reference make_ref(std::int32_t in_ordinal, bool in_outlier) {
  ai_light_reference l_ref{};
  l_ref.uuid_id_          = core_set::get_set().get_uuid();
  l_ref.ordinal_          = in_ordinal;
  l_ref.width_            = 1920;
  l_ref.height_           = 1080;
  l_ref.stats_confidence_ = 0.8;
  l_ref.is_outlier_       = in_outlier;
  if (in_outlier) l_ref.excluded_reason_ = "人工排除";

  reference_image_stats_t l_stats{};
  l_stats.space_                     = "display_srgb";
  l_stats.luma_percentiles_.p10_     = 0.06;
  l_stats.luma_percentiles_.p50_     = 0.43;
  l_stats.luma_percentiles_.p90_     = 0.76;
  l_stats.contrast_                  = 0.70;
  l_stats.white_balance_kelvin_      = 4100.0;
  l_stats.clip_rate_.high_           = 0.005;
  l_stats.clip_rate_.low_            = 0.010;
  l_stats.retouch_score_             = 0.2;
  l_stats.palette_ = std::vector<image_palette_entry_t>{
      image_palette_entry_t{vec3_t{0.10, 0.12, 0.20}, 0.50},
      image_palette_entry_t{vec3_t{0.90, 0.70, 0.40}, 0.18},
  };
  l_ref.set_stats(l_stats);
  return l_ref;
}

/// 一份**合法**的模型输出（各用例按需改其中一个字段）。
/// 形状照 build_spec_json_schema() 的契约：intent / targets / conflicts 三键都在。
nlohmann::json make_llm_json() {
  nlohmann::json l_clip{};
  l_clip["high"] = nlohmann::json::array({0.0, 0.01});
  l_clip["low"]  = nlohmann::json::array({0.0, 0.02});

  nlohmann::json l_luma{};
  l_luma["p10"] = nlohmann::json::array({0.05, 0.08});
  l_luma["p50"] = nlohmann::json::array({0.40, 0.46});
  l_luma["p90"] = nlohmann::json::array({0.72, 0.80});

  nlohmann::json l_grading{};
  l_grading["vignette"]      = "subtle";
  l_grading["lifted_blacks"] = false;
  l_grading["bloom"]         = "none";

  nlohmann::json l_targets{};
  l_targets["contrast"]             = nlohmann::json::array({0.55, 0.75});
  l_targets["white_balance_kelvin"] = nlohmann::json::array({4000.0, 4200.0});
  l_targets["luma_percentiles"]     = l_luma;
  l_targets["clip_rate"]            = l_clip;
  l_targets["grading_hints"]        = l_grading;

  nlohmann::json l_intent{};
  l_intent["atmosphere"]             = nlohmann::json::array({"冷", "孤独"});
  l_intent["time_of_day"]            = "night";
  l_intent["weather"]                = "clear";
  l_intent["source_motivation"]      = nlohmann::json::array({"street_lamp"});
  l_intent["character_personality"]  = nlohmann::json::array();
  l_intent["story_context"]          = "深夜独行";
  l_intent["setting"]                = "空旷街道";

  nlohmann::json l_json{};
  l_json["intent"]  = l_intent;
  l_json["targets"] = l_targets;
  // 故意不造 "conflicts" 键：它已从 schema 删除（决定 D7），模型的输出里不该再出现，
  // 而 map_spec_result 也不读它 —— 冲突全部由聚合器判定。
  return l_json;
}

/// Boost.Test 打不出普通 enum（没有 operator<<），打印失败就是 static_assert
/// —— 转成 magic_enum 的名字再比，顺便让失败信息可读。用法同 ai_light_state.cpp。
template <typename E>
std::string en(E in_e) {
  return std::string{magic_enum::enum_name(in_e)};
}

bool has_interval(const nlohmann::json& in_json, const char* in_key, const interval_t& in_want) {
  if (!in_json.contains(in_key) || !in_json.at(in_key).is_array() || in_json.at(in_key).size() != 2) return false;
  return std::abs(in_json.at(in_key).at(0).get<double>() - in_want.low_) < 1e-9 &&
         std::abs(in_json.at(in_key).at(1).get<double>() - in_want.high_) < 1e-9;
}

}  // namespace

BOOST_AUTO_TEST_SUITE(ai_light_spec_prompt)

// ===========================================================================
// schema：交给模型的输出契约
// ===========================================================================

BOOST_AUTO_TEST_CASE(json_schema_matches_cpp_enums) {
  const auto l_schema = build_spec_json_schema();

  // 自由文本枚举（不是 C++ 枚举，所以只能对照 §5.5 的表）——hardness 恰好三档。
  const auto l_hardness =
      l_schema.at("properties").at("intent").at("properties").at("character_personality").at("items").at("properties")
          .at("lighting_language").at("properties").at("hardness").at("enum")
          .get<std::vector<std::string>>();
  auto l_hardness_sorted = l_hardness;
  std::sort(l_hardness_sorted.begin(), l_hardness_sorted.end());
  const std::vector<std::string> l_hardness_want{"hard", "soft", "very_soft"};
  BOOST_TEST(l_hardness_sorted == l_hardness_want, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(json_schema_is_strict_object) {
  const auto l_schema = build_spec_json_schema();

  BOOST_TEST(l_schema.at("type").get<std::string>() == "object");
  BOOST_TEST(l_schema.at("additionalProperties").get<bool>() == false);

  // 根上恰好这两个键 —— 多一个都意味着「服务端没打算读它」（palette/ownership 就是这么被排除的）。
  // 原先还有第三个键 "conflicts"，已删除（决定 D7）：冲突只有一个权威来源，就是聚合器写进
  // reference_stats_.conflicts_ 的那份统计判断，模型自报既同质又无法校验。
  const std::vector<std::string> l_root_keys{"intent", "targets"};
  // properties 是 object，不能 get<std::vector<std::string>>()（会抛 type_error.302）→ 显式收集键名。
  std::vector<std::string> l_keys{};
  for (auto l_it = l_schema.at("properties").begin(); l_it != l_schema.at("properties").end(); ++l_it)
    l_keys.emplace_back(l_it.key());
  std::sort(l_keys.begin(), l_keys.end());
  BOOST_TEST(l_keys == l_root_keys, boost::test_tools::per_element());

  const auto l_targets = l_schema.at("properties").at("targets");
  // 调色板与所有权划分是**服务端口径**：颜色只能来自统计，划分是产品边界（§6.6），
  // 不该让模型每轮重新发明。所以它们不在 schema 里 —— 这条断言就是那个决定本身。
  BOOST_TEST(!l_targets.at("properties").contains("palette"));
  BOOST_TEST(!l_targets.at("properties").contains("ownership"));
  BOOST_TEST(!l_targets.at("properties").contains("space"));

  // 区间是定长 2 元数组，与 interval_t::from_json 的校验（非 2 元素抛 type_error）对齐。
  const auto l_contrast = l_targets.at("properties").at("contrast");
  BOOST_TEST(l_contrast.at("type").get<std::string>() == "array");
  BOOST_TEST(l_contrast.at("minItems").get<int>() == 2);
  BOOST_TEST(l_contrast.at("maxItems").get<int>() == 2);
  BOOST_TEST(l_contrast.at("items").at("type").get<std::string>() == "number");

  // strict 结构化输出要求把 required 列全：targets 的五个子键一个都不能少。
  const std::vector<std::string> l_targets_required{
      "clip_rate", "contrast", "grading_hints", "luma_percentiles", "white_balance_kelvin"};
  auto l_req = l_targets.at("required").get<std::vector<std::string>>();
  std::sort(l_req.begin(), l_req.end());
  BOOST_TEST(l_req == l_targets_required, boost::test_tools::per_element());
}

// ===========================================================================
// input_text：交给模型的正文
// ===========================================================================

BOOST_AUTO_TEST_CASE(input_text_carries_prompt_and_stats) {
  const auto l_agg = make_agg();

  const auto l_excluded = make_ref(1, true);
  spec_generation_request_t l_req{};
  l_req.prompt_text_ = "冷、硬、孤独，夜里一个人站在空旷街道上";
  l_req.references_  = {make_ref(0, false), l_excluded};

  const auto l_text = build_spec_input_text(l_req, l_agg);

  // ① 提示词原文必须原样出现：它是权威意图，被摘要或改写就等于换了输入。
  BOOST_TEST(l_text.find("冷、硬、孤独") != std::string::npos);
  // ② 逐图统计：两张图都要出现（被排除的要出现，好让模型知道被否掉的是什么）
  BOOST_TEST(l_text.find("#0") != std::string::npos);
  BOOST_TEST(l_text.find("#1") != std::string::npos);
  // 被人工排除的图必须被**标注**出来，否则模型会把它的统计当有效样本
  BOOST_TEST(l_text.find("已人工排除") != std::string::npos);
  // ③ 聚合目标：两图统计的中位数与区间
  BOOST_TEST(l_text.find("0.55") != std::string::npos);  // contrast 下界
  BOOST_TEST(l_text.find("0.75") != std::string::npos);  // contrast 上界
  BOOST_TEST(l_text.find("median") != std::string::npos);
  // ④ 没有上一版时不该出现「重出」那段
  BOOST_TEST(l_text.find("上一版") == std::string::npos);

  // 有上一版时：把上一版的 intent 摆出来，并明确「除非有要求否则保持一致」。
  lighting_spec_t l_prev{};
  l_prev.intent_.atmosphere_  = std::vector<std::string>{"温暖", "怀旧"};
  l_prev.intent_.time_of_day_ = "dusk";
  l_req.previous_spec_        = l_prev;
  const auto l_text2          = build_spec_input_text(l_req, l_agg);
  BOOST_TEST(l_text2.find("上一版") != std::string::npos);
  BOOST_TEST(l_text2.find("温暖") != std::string::npos);
  BOOST_TEST(l_text2.find("dusk") != std::string::npos);
}

// ===========================================================================
// request：交给方舟的信封
// ===========================================================================

BOOST_AUTO_TEST_CASE(build_request_omits_images_when_empty) {
  const auto l_agg = make_agg();

  spec_generation_request_t l_empty{};
  l_empty.prompt_text_ = "任意";
  const auto l_req_empty = build_spec_request(l_empty, l_agg);
  // 信封现在是强类型结构体：键名与类型由 inference_request_t 在编译期钉住，这里直接读字段。
  BOOST_TEST(l_req_empty.images_.empty());
  BOOST_TEST(l_req_empty.json_schema_name_ == "lighting_spec");
  BOOST_TEST(l_req_empty.json_schema_strict_.value_or(false) == true);
  BOOST_TEST(l_req_empty.image_detail_.value_or("") == "high");
  BOOST_TEST(l_req_empty.temperature_.value_or(0.0) == 0.2);
  // 不发 thinking：缺省就是 disabled，而开 thinking 会让 output[0] 变成 reasoning 项
  // （§15.6 第 2 条）——显式断言「我们没把它发出去」比依赖缺省值可靠。
  BOOST_TEST(!l_req_empty.thinking_.has_value());
  // 不发 max_output_tokens：截断会让 result_json_ 变成半个 JSON，失败面与「schema 不合规」分不开
  BOOST_TEST(!l_req_empty.max_output_tokens_.has_value());
  BOOST_TEST(l_req_empty.instructions_.value_or("").find("提示词") != std::string::npos);

  // 线格式仍然要钉死：落库的 data_request_ 就是这份 json，旧行也必须还能读回来。
  // 注意用拷贝初始化 `= l_req` 而不是 `nlohmann::json{l_req}`：花括号会走 initializer_list
  // 构造，把信封裹成一个单元素数组。
  const nlohmann::json l_json_empty = l_req_empty;
  // 空数组不能发：方舟对空 images 的处理没确认过，而「没有图」与「图是空列表」不该被它猜。
  BOOST_TEST(!l_json_empty.contains("images"));
  BOOST_TEST(l_json_empty.at("json_schema_name").get<std::string>() == "lighting_spec");
  BOOST_TEST(l_json_empty.at("json_schema_strict").get<bool>() == true);
  BOOST_TEST(l_json_empty.at("image_detail").get<std::string>() == "high");
  BOOST_TEST(!l_json_empty.contains("thinking"));
  BOOST_TEST(!l_json_empty.contains("max_output_tokens"));
  // 落库再读回来（结构体 -> json -> 结构体 -> json）必须是同一份信封：
  // 这条守着「旧行可读、新行形状不变」。
  const http::seedance2::inference_request_t l_back = l_json_empty.get<http::seedance2::inference_request_t>();
  const nlohmann::json l_json_back                  = l_back;
  BOOST_TEST(l_json_back == l_json_empty);

  // 全部图都被人工排除 → 仍然不发 images（排除的图不该再进入模型的视觉输入）
  const auto l_only_outlier   = make_ref(0, true);
  l_empty.references_         = {l_only_outlier};
  const auto l_req_outlier    = build_spec_request(l_empty, l_agg);
  const nlohmann::json l_json_outlier = l_req_outlier;
  BOOST_TEST(l_req_outlier.images_.empty());
  BOOST_TEST(!l_json_outlier.contains("images"));

  // 只有未被排除的图才发，且发的是**站点内相对路径**（由 to_public_url 补前缀 + 签名），
  // 而不是磁盘上的绝对路径 —— 后者既取不到也泄露部署结构。
  const auto l_ok     = make_ref(0, false);
  l_empty.references_ = {l_ok, l_only_outlier};
  const auto l_req_ok = build_spec_request(l_empty, l_agg);
  const nlohmann::json l_json_ok = l_req_ok;
  BOOST_TEST(l_req_ok.images_.size() == 1);
  BOOST_TEST(l_json_ok.contains("images"));
  BOOST_TEST(l_json_ok.at("images").size() == 1);
  const auto l_images = l_req_ok.images_;
  BOOST_TEST(l_images.size() == 1);
  // 期望值在这里独立拼出来（而不是调 ai_light.h 的 reference_image_url）：
  // 测试要钉的是**线格式**，跟着实现走就测不出格式漂移了。
  const auto l_expected_url =
      fmt::format("/api/doodle/ai/light/reference/{}/image", boost::uuids::to_string(l_ok.uuid_id_));
  BOOST_TEST(l_images.at(0) == l_expected_url);
  BOOST_TEST(l_images.at(0).find("/api/doodle/ai/light/reference/") == 0);
}

// 生产端 -> 消费端：信封必须能原样翻成方舟 Responses 请求体。
// 这一条是这次结构体化的目的本身 —— 两侧一旦对同一个键的理解分叉，这里就会红。
BOOST_AUTO_TEST_CASE(build_request_reaches_ark_body) {
  const auto l_agg = make_agg();
  const auto l_ok  = make_ref(0, false);

  spec_generation_request_t l_request{};
  l_request.prompt_text_ = "暖一点, 柔一点";
  l_request.references_  = {l_ok};

  const auto l_req  = build_spec_request(l_request, l_agg);
  const auto l_body = http::seedance2::seedance2_client::build_inference_body(l_req);

  // 提示词与素材都不留在方舟侧
  BOOST_TEST(l_body.at("store").get<bool>() == false);
  // 结构化输出必须是平铺的 text.format（§15.6 第 4 条），且 schema 就是我们的 schema 契约
  const auto& l_format = l_body.at("text").at("format");
  BOOST_TEST(l_format.at("type").get<std::string>() == "json_schema");
  BOOST_TEST(l_format.at("name").get<std::string>() == "lighting_spec");
  BOOST_TEST(l_format.at("strict").get<bool>() == true);
  BOOST_TEST(l_format.at("schema").at("properties").contains("intent"));
  BOOST_TEST(l_format.at("schema").at("properties").contains("targets"));
  // 契约里不该再有 conflicts：冲突由服务端聚合器判定，不问模型（决定 D7）。
  BOOST_TEST(!l_format.at("schema").at("properties").contains("conflicts"));
  // 不发 max_output_tokens：全字段 required + 长度不可预估 = 截断成半个 JSON 的失败面
  BOOST_TEST(!l_body.contains("max_output_tokens"));
  // 不发 reasoning_effort（信封里没设）
  BOOST_TEST(!l_body.contains("reasoning"));
  // thinking 缺省 disabled → 显式发 thinking.type=disabled
  BOOST_TEST(l_body.at("thinking").at("type").get<std::string>() == "disabled");
  BOOST_TEST(l_body.at("temperature").get<double>() == 0.2);
  BOOST_TEST(l_body.at("model").get<std::string>() == std::string{doodle_config::g_ark_inference_model});

  // input[0] 是 message，content 依次是文本与图片
  const auto& l_item = l_body.at("input").at(0);
  BOOST_TEST(l_item.at("type").get<std::string>() == "message");
  BOOST_TEST(l_item.at("role").get<std::string>() == "user");
  const auto& l_content = l_item.at("content");
  BOOST_TEST(l_content.at(0).at("type").get<std::string>() == "input_text");
  BOOST_TEST(l_content.at(0).at("text").get<std::string>().find("暖一点") != std::string::npos);
  BOOST_TEST(l_content.at(1).at("type").get<std::string>() == "input_image");
  // Responses 的 image_url 是**字符串**（生成 API 才是 {"url": ...} 对象）
  BOOST_TEST(l_content.at(1).at("image_url").is_string());
  BOOST_TEST(l_content.at(1).at("detail").get<std::string>() == "high");
  BOOST_TEST(
      l_content.at(1).at("image_url").get<std::string>() ==
      fmt::format("/api/doodle/ai/light/reference/{}/image", boost::uuids::to_string(l_ok.uuid_id_))
  );

  // 没有参考图时：仍然能翻，只是 content 里只有文本
  l_request.references_.clear();
  const auto l_no_image = http::seedance2::seedance2_client::build_inference_body(
      build_spec_request(l_request, l_agg)
  );
  BOOST_TEST(l_no_image.at("input").at(0).at("content").size() == 1);
}

// ===========================================================================
// map_spec_result：模型输出 → LightingSpec
// ===========================================================================

BOOST_AUTO_TEST_CASE(map_result_pins_server_owned_fields) {
  const auto l_agg = make_agg();
  const auto l_ret = map_spec_result(make_llm_json(), l_agg);

  // 身份与版本由服务端写死：模型没有、也不该有发言权。
  BOOST_TEST(l_ret.spec_.schema_version_ == std::string{g_spec_schema_version});
  BOOST_TEST(l_ret.spec_.human_confirmed_ == false);
  BOOST_TEST(l_ret.spec_.confirmed_by_.is_nil() == true);
  BOOST_TEST(l_ret.spec_.confirmed_at_.has_value() == false);

  // 调色板整块来自统计：模型输出里根本没有 palette 键，所以它必然是聚合结果本身。
  BOOST_TEST(l_ret.spec_.targets_.palette_.size() == l_agg.targets_.palette_.size());
  BOOST_TEST(std::abs(l_ret.spec_.targets_.palette_.at(0).color_.x_ - 0.10) < 1e-9);
  BOOST_TEST(std::abs(l_ret.spec_.targets_.palette_.at(0).share_.low_ - 0.40) < 1e-9);
  // ownership 同理：它是产品边界（§6.6），不是模型每轮重新发明的划分。
  BOOST_TEST(l_ret.spec_.targets_.ownership_.lighting_.size() == 2);
  BOOST_TEST(l_ret.spec_.targets_.ownership_.grading_.at(0) == "contrast");
  BOOST_TEST(l_ret.spec_.targets_.space_ == "display_srgb");

  // reference_stats 整块照抄聚合结果，且 usable_for_physical_solve_ 恒 false
  // （参考图是全局调色为主，不足以做物理逆解 —— §5.3）。
  BOOST_TEST(l_ret.reference_stats_.image_count_ == 3);
  BOOST_TEST(l_ret.reference_stats_.usable_for_physical_solve_ == false);

  // 有逐维置信度 → 事实上的 stats_only（决定 D4）。
  BOOST_TEST(en(l_ret.spec_.degrade_level_) == en(spec_degrade_level::stats_only));

  // 模型照抄统计值 → 取值与 stats 一致（没有改写）。
  BOOST_TEST(std::abs(l_ret.spec_.targets_.contrast_.low_ - l_agg.targets_.contrast_.low_) < 1e-9);
}

BOOST_AUTO_TEST_CASE(map_result_without_stats_degrades_to_prompt_only) {
  // 调用方可能因为 references_have_stats() 判成 stats_only，但聚合器一个维度都算不出来
  // ——「有 stats 行但置信度全 0」「全部图被人工排除」都会这样。落库的降级级别以事实为准。
  const auto l_agg = make_agg(false);
  const auto l_ret = map_spec_result(make_llm_json(), l_agg);
  BOOST_TEST(en(l_ret.spec_.degrade_level_) == en(spec_degrade_level::prompt_only));
}

BOOST_AUTO_TEST_CASE(map_result_prompt_overrides_stats_target) {
  auto l_agg = make_agg();
  auto l_llm = make_llm_json();
  // 提示词说「要更硬的高对比」——模型给出一段与统计完全不相交的区间。
  l_llm["targets"]["contrast"] = nlohmann::json::array({0.90, 0.95});

  const auto l_ret = map_spec_result(l_llm, l_agg);

  // §6.3：提示词明确 + 参考冲突 → 提示词胜出。
  BOOST_TEST(std::abs(l_ret.spec_.targets_.contrast_.low_ - 0.90) < 1e-9);
  BOOST_TEST(std::abs(l_ret.spec_.targets_.contrast_.high_ - 0.95) < 1e-9);
}

BOOST_AUTO_TEST_CASE(map_result_echo_keeps_reference_stats_values) {
  const auto l_agg = make_agg();
  // 模型照抄了统计值 → 取值不动。这是「提示词覆盖了参考」这个信号的另一半：
  // 取值与 stats 相同就说明模型认可了参考图统计。
  const auto l_ret = map_spec_result(make_llm_json(), l_agg);

  // 夹具里 make_llm_json() 的 targets 与 make_agg().targets_ 逐字段同值,
  // 所以这组断言同时验了「照抄不改写」: 模型值与统计值同值时, 最终取值仍是那个值.
  BOOST_TEST(std::abs(l_ret.spec_.targets_.contrast_.low_ - 0.55) < 1e-9);
  BOOST_TEST(std::abs(l_ret.spec_.targets_.contrast_.high_ - 0.75) < 1e-9);
  BOOST_TEST(std::abs(l_ret.spec_.targets_.white_balance_kelvin_.low_ - 4000.0) < 1e-9);
  BOOST_TEST(std::abs(l_ret.spec_.targets_.white_balance_kelvin_.high_ - 4200.0) < 1e-9);

  // 统计口径的东西不因模型输出而改变（见文件头第 9-10 行）——模型根本没被允许给这些键.
  BOOST_TEST(l_ret.spec_.targets_.palette_.size() == 2);
  // 不能用花括号初始化列表: Boost.Test 会把它当惰性表达式解析 (C3329 / C2059).
  const std::vector<std::string> l_ownership_want{"contrast", "vignette"};
  BOOST_TEST(l_ret.spec_.targets_.ownership_.grading_ == l_ownership_want);
}

BOOST_AUTO_TEST_CASE(map_result_falls_back_on_illegal_enum) {
  auto l_llm = make_llm_json();
  // 自由文本枚举必须被服务端拦下。裸 get<std::string>() 不会报错，
  // 而 grading_hints 直接进调色参数，非法值会一路传到下游。
  l_llm["targets"]["grading_hints"]["vignette"] = "super_strong";
  // 性格的灯光语言同样要校验。
  nlohmann::json l_person{};
  l_person["subject"] = "神明";
  l_person["trait"]   = "超然";
  nlohmann::json l_lang{};
  l_lang["key_fill_ratio"] = nlohmann::json::array({1.0, 2.0});
  l_lang["hardness"]       = "super_hard";
  l_lang["position_bias"]  = "top";
  l_lang["kelvin_bias"]    = "cold";
  l_person["lighting_language"] = l_lang;
  l_llm["intent"]["character_personality"] = nlohmann::json::array({l_person});

  const auto l_ret = map_spec_result(l_llm, make_agg());

  // 非法值被丢弃 → 保留统计推导出来的值，而不是半截的模型值。
  BOOST_TEST(l_ret.spec_.targets_.grading_hints_.vignette_ == "subtle");
  // lifted_blacks 是**值合法但取值相反**的字段: 模型给了 false, 统计推导出的是 true.
  // 按「提示词优先」(§6.3) 采用模型的 false —— 这与上面 vignette 的「整块丢弃」不同:
  // 校验只管值合不合法, 不管它与统计一致不一致.
  BOOST_TEST(l_ret.spec_.targets_.grading_hints_.lifted_blacks_ == false);

  // 性格行本身有信息量（subject/trait 非空）→ 保留；但两个非法枚举字段都被清空。
  BOOST_TEST(l_ret.spec_.intent_.character_personality_.size() == 1);
  const auto& l_cp = l_ret.spec_.intent_.character_personality_.at(0);
  BOOST_TEST(l_cp.subject_ == "神明");
  BOOST_TEST(l_cp.lighting_language_.hardness_.empty());
  BOOST_TEST(l_cp.lighting_language_.position_bias_.empty());
  // kelvin_bias 的允许值是 cool/warm/neutral，所以 "cold" 也不合法
  BOOST_TEST(l_cp.lighting_language_.kelvin_bias_.empty());
  BOOST_TEST(l_cp.lighting_language_.key_fill_ratio_.has_value());
}

BOOST_AUTO_TEST_CASE(map_result_rejects_non_object) {
  const auto l_agg = make_agg();

  // null 是最常见的一种：模型回答了自然语言，没按 json_schema 输出
  // （parse_inference_response 里 json::parse 失败就留 null）。
  bool l_null_threw = false;
  try {
    map_spec_result(nlohmann::json{}, l_agg);
  } catch (const doodle_error& in_err) {
    l_null_threw = true;
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::bad_gateway);
  }
  BOOST_TEST(l_null_threw);

  bool l_array_threw = false;
  try {
    map_spec_result(nlohmann::json::array({1, 2, 3}), l_agg);
  } catch (const doodle_error& in_err) {
    l_array_threw = true;
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::bad_gateway);
  }
  BOOST_TEST(l_array_threw);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace doodle::ai_light
