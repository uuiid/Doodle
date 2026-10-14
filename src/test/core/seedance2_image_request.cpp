//
// 中转站(Grsai)图片生成请求体的建模测试
//
// 盯的是 image_request.h 这个**跨进程契约**上「写错了不会在本进程报错, 只会得到上游一句
// generate failed」的那几处:
//
//   1) 键名必须与两份 Apifox 文档逐字一致(aspectRatio / imageSize / replyType 是驼峰),
//      大小写错一个字符上游就忽略或报错, 而本地怎么跑都是通的;
//   2) 选填键必须**有值才发**: 未设置与设成空必须在类型层面分开(images/imageSize/
//      quality/background/mask/replyType), 多发一个空键在上游没有文档承诺;
//   3) `imageSize` 是 "1K"/"2K"/"4K" 这种带 K 的字符串, 不是 1/2/4 —— 字符串原样进 JSON,
//      本工程不做任何大小写规整, 所以这里钉的就是「原样」;
//   4) from_json 必须**宽容**: data_request 那一列是三后端共用的, 方舟形态的行
//      (只有 model/resolution/content)不该让读中转站形态这件事炸掉。
//
// 这几个字段刻意没有枚举(imageSize/quality/background/replyType), 理由见 image_request.h
// 的「为什么这里没有枚举」—— 取值域在上游。因此也没有「枚举名↔JSON 名」的映射可测。
//
// 都不联网、不需要 app_base。
//

#include <doodle_core/metadata/seedance2/image_request.h>

#include <boost/test/unit_test.hpp>

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

BOOST_AUTO_TEST_SUITE(seedance2_image_request)

namespace {
using namespace doodle::seedance2;

/// 一份「nano-banana 形态」的完整请求: 文档 example 的键集(不含 replyType, 提交时另注入)
transfer_station_request_t nano_banana_request() {
  transfer_station_request_t l_req{};
  l_req.model_            = "nano-banana-2";
  l_req.prompt_           = "生成一张边牧与古牧正在抖音直播间直播带货截图";
  l_req.aspect_ratio_     = "1:1";
  l_req.image_size_       = "1K";
  l_req.reference_images_ = std::vector<std::string>{"/api/seedance2/pictures/11111111-1111-1111-1111-111111111111.png"};
  return l_req;
}

/// 一份「gpt-image 形态」的请求: 没有 imageSize(它用 aspectRatio 承载像素值), 有 quality/mask
transfer_station_request_t gpt_image_request() {
  transfer_station_request_t l_req{};
  l_req.model_        = "gpt-image-2-vip";
  l_req.prompt_       = "把背景换成纯白";
  l_req.aspect_ratio_ = "2048x2048";
  l_req.quality_      = "medium";
  l_req.mask_         = "/api/seedance2/pictures/22222222-2222-2222-2222-222222222222.png";
  return l_req;
}
}  // namespace

// ---------------------------------------------------------------------------
// 键名与取值: 两份文档的逐字段对照
// ---------------------------------------------------------------------------

// nano-banana 文档(452392911e0)的请求体形状
BOOST_AUTO_TEST_CASE(nano_banana_request_matches_api_doc) {
  const auto l_json = nlohmann::json{nano_banana_request()}.dump();

  // 键名逐字: 文档写的是驼峰 aspectRatio / imageSize
  BOOST_CHECK(l_json.find("\"aspectRatio\"") != std::string::npos);
  BOOST_CHECK(l_json.find("\"imageSize\"") != std::string::npos);
  BOOST_CHECK(l_json.find("\"model\"") != std::string::npos);
  BOOST_CHECK(l_json.find("\"prompt\"") != std::string::npos);
  BOOST_CHECK(l_json.find("\"images\"") != std::string::npos);

  const auto l_out = nlohmann::json{nano_banana_request()};
  BOOST_CHECK_EQUAL(l_out.at("model").get<std::string>(), "nano-banana-2");
  BOOST_CHECK_EQUAL(l_out.at("aspectRatio").get<std::string>(), "1:1");
  // 这是 "1K" 不是 1 —— 文档的枚举就是带 K 的字符串
  BOOST_CHECK_EQUAL(l_out.at("imageSize").get<std::string>(), "1K");
  BOOST_CHECK_EQUAL(l_out.at("images").size(), 1U);
}

// gpt-image 文档(452409160e0)的请求体形状: 同一套键 + quality / mask
BOOST_AUTO_TEST_CASE(gpt_image_request_matches_api_doc) {
  const auto l_out = nlohmann::json{gpt_image_request()};
  BOOST_CHECK_EQUAL(l_out.at("model").get<std::string>(), "gpt-image-2-vip");
  // gpt-image 系列没有 imageSize, 分辨率放在 aspectRatio 的像素值里
  BOOST_CHECK_EQUAL(l_out.contains("imageSize"), false);
  BOOST_CHECK_EQUAL(l_out.at("aspectRatio").get<std::string>(), "2048x2048");
  BOOST_CHECK_EQUAL(l_out.at("quality").get<std::string>(), "medium");
  BOOST_CHECK(l_out.at("mask").is_string());
}

// quality 的 "auto" 走字符串路径, 原样进 JSON —— 早先用枚举时它叫 auto_(C++ 关键字冲突),
// 现在没有这一层映射, 但「发出去的是 auto 不是 auto_」这条契约仍然值得钉住
BOOST_AUTO_TEST_CASE(quality_auto_is_sent_verbatim) {
  transfer_station_request_t l_req{};
  l_req.model_   = "gpt-image-2";
  l_req.prompt_  = "x";
  l_req.quality_ = "auto";

  BOOST_CHECK_EQUAL(nlohmann::json{l_req}.at("quality").get<std::string>(), "auto");
  // 反过来读也必须是同一个值(不能读成 auto_ 再发出去)
  BOOST_CHECK_EQUAL(nlohmann::json{l_req}.get<transfer_station_request_t>().quality_, "auto");
}

// imageSize 是带 K 的字符串, 且必须**逐字**原样发出 —— 剧集授权表里登记的就是这个串,
// 大小写或后缀被规整过就会对不上账
BOOST_AUTO_TEST_CASE(image_size_is_sent_verbatim) {
  const auto l_json_of = [](std::string in_size) {
    transfer_station_request_t l_req{};
    l_req.model_      = "nano-banana-2";
    l_req.prompt_     = "x";
    l_req.image_size_ = std::move(in_size);
    return nlohmann::json{l_req}.at("imageSize").get<std::string>();
  };
  BOOST_CHECK_EQUAL(l_json_of("1K"), "1K");
  BOOST_CHECK_EQUAL(l_json_of("2K"), "2K");
  BOOST_CHECK_EQUAL(l_json_of("4K"), "4K");
  // 不做白名单: 不是本工程认识的取值也照发, 由上游拒绝(取值域在上游)
  BOOST_CHECK_EQUAL(l_json_of("3K"), "3K");
  // 也不做大小写规整: "1k" 发出去还是 "1k"
  BOOST_CHECK_EQUAL(l_json_of("1k"), "1k");
}

// background 与 replyType 同样是原样字符串
BOOST_AUTO_TEST_CASE(background_and_reply_type_are_sent_verbatim) {
  transfer_station_request_t l_req{};
  l_req.model_      = "gpt-image-2.5-flare";
  l_req.prompt_     = "x";
  l_req.background_ = "transparent";
  l_req.reply_type_ = "async";
  BOOST_CHECK_EQUAL(nlohmann::json{l_req}.at("background").get<std::string>(), "transparent");
  BOOST_CHECK_EQUAL(nlohmann::json{l_req}.at("replyType").get<std::string>(), "async");
}

// ---------------------------------------------------------------------------
// 选填键: 有值才发
// ---------------------------------------------------------------------------

// 一份只有必填项的请求, 不得出现任何选填键
BOOST_AUTO_TEST_CASE(optional_keys_are_omitted_when_unset) {
  transfer_station_request_t l_req{};
  l_req.model_  = "nano-banana-2";
  l_req.prompt_ = "只有提示词";

  const auto l_out = nlohmann::json{l_req};
  BOOST_CHECK_EQUAL(l_out.contains("images"), false);
  BOOST_CHECK_EQUAL(l_out.contains("aspectRatio"), false);
  BOOST_CHECK_EQUAL(l_out.contains("imageSize"), false);
  BOOST_CHECK_EQUAL(l_out.contains("quality"), false);
  BOOST_CHECK_EQUAL(l_out.contains("background"), false);
  BOOST_CHECK_EQUAL(l_out.contains("mask"), false);
  BOOST_CHECK_EQUAL(l_out.contains("replyType"), false);
  // 必填键无条件出现
  BOOST_CHECK_EQUAL(l_out.contains("model"), true);
  BOOST_CHECK_EQUAL(l_out.contains("prompt"), true);
}

// 「设成空」与「未设置」必须同义(都不发), 不能发一个空数组/空串出去
BOOST_AUTO_TEST_CASE(empty_values_do_not_emit_keys) {
  transfer_station_request_t l_req{};
  l_req.model_            = "nano-banana-2";
  l_req.prompt_           = "x";
  l_req.aspect_ratio_     = "";   // 显式赋空串
  l_req.mask_             = "";
  l_req.reference_images_ = std::vector<std::string>{};  // 显式赋空数组
  BOOST_CHECK_EQUAL(l_req.has_reference_images(), false);

  const auto l_out = nlohmann::json{l_req};
  BOOST_CHECK_EQUAL(l_out.contains("images"), false);
  BOOST_CHECK_EQUAL(l_out.contains("aspectRatio"), false);
  BOOST_CHECK_EQUAL(l_out.contains("mask"), false);
}

BOOST_AUTO_TEST_CASE(has_reference_images_detects_references) {
  BOOST_CHECK_EQUAL(nano_banana_request().has_reference_images(), true);
  BOOST_CHECK_EQUAL(gpt_image_request().has_reference_images(), false);  // 只有 mask, 不算参考图
}

// ---------------------------------------------------------------------------
// from_json: 宽容 + 严格枚举
// ---------------------------------------------------------------------------

// 方舟生成形态的行(只有 model/resolution/content)缺 prompt 与尺寸 —— 按调用约定它根本
// 不该走到这个读法(判后端是调用方的事), 走到这里就是误用, 当场抛
BOOST_AUTO_TEST_CASE(from_json_rejects_ark_generation_shape) {
  const auto l_ark = nlohmann::json{
      {"model", "doubao-seedance-1-0-pro-250528"},
      {"resolution", "1080p"},
      {"content", nlohmann::json::array({nlohmann::json{{"type", "text"}, {"text", "一只边牧"}}})}
  };
  transfer_station_request_t l_req{};
  BOOST_CHECK_THROW(l_req = l_ark.get<transfer_station_request_t>(), nlohmann::json::type_error);
}

// 推理形态的行(Responses 信封)同样抛
BOOST_AUTO_TEST_CASE(from_json_rejects_inference_shape) {
  const auto l_infer = nlohmann::json{
      {"model", "doubao-seed-1-6"},
      {"input_text", "暖一点, 柔一点"},
      {"instructions", "你是灯光指导"}
  };
  transfer_station_request_t l_req{};
  BOOST_CHECK_THROW(l_req = l_infer.get<transfer_station_request_t>(), nlohmann::json::type_error);
}

// 非对象输入(这列是 json, 历史行里可能有 null / 数组)也抛, 不是静默给个空结构体
BOOST_AUTO_TEST_CASE(from_json_rejects_non_object) {
  transfer_station_request_t l_a{};
  BOOST_CHECK_THROW(l_a = nlohmann::json{}.get<transfer_station_request_t>(), nlohmann::json::type_error);
  transfer_station_request_t l_b{};
  BOOST_CHECK_THROW(l_b = nlohmann::json::array({1, 2, 3}).get<transfer_station_request_t>(), nlohmann::json::type_error);
}

// 必填: model 与 prompt 缺一不可
BOOST_AUTO_TEST_CASE(from_json_requires_model_and_prompt) {
  const auto l_no_model = nlohmann::json{{"prompt", "x"}, {"aspectRatio", "1:1"}};
  BOOST_CHECK_THROW(l_no_model.get<transfer_station_request_t>(), nlohmann::json::type_error);

  const auto l_no_prompt = nlohmann::json{{"model", "nano-banana-2"}, {"aspectRatio", "1:1"}};
  BOOST_CHECK_THROW(l_no_prompt.get<transfer_station_request_t>(), nlohmann::json::type_error);

  // 键在但类型不对, 等同于缺失
  const auto l_bad_model = nlohmann::json{{"model", 7}, {"prompt", "x"}, {"aspectRatio", "1:1"}};
  BOOST_CHECK_THROW(l_bad_model.get<transfer_station_request_t>(), nlohmann::json::type_error);

  const auto l_empty_string_is_present = nlohmann::json{{"model", ""}, {"prompt", ""}, {"aspectRatio", "1:1"}};
  BOOST_REQUIRE_NO_THROW(l_empty_string_is_present.get<transfer_station_request_t>());
}

// 必填: imageSize 与 aspectRatio 必居其一
BOOST_AUTO_TEST_CASE(from_json_requires_a_resolution_key) {
  const auto l_neither = nlohmann::json{{"model", "nano-banana-2"}, {"prompt", "x"}};
  BOOST_CHECK_THROW(l_neither.get<transfer_station_request_t>(), nlohmann::json::type_error);

  // 两者都有是允许的: nano-banana 与 gpt-image 的键集本就可能同时出现
  const auto l_both = nlohmann::json{{"model", "nano-banana-2"}, {"prompt", "x"}, {"imageSize", "1K"}, {"aspectRatio", "1:1"}};
  transfer_station_request_t l_req{};
  BOOST_REQUIRE_NO_THROW(l_req = l_both.get<transfer_station_request_t>());
  BOOST_CHECK_EQUAL(l_req.image_size_, "1K");
  BOOST_CHECK_EQUAL(l_req.aspect_ratio_, "1:1");

  // 单给 aspectRatio(gpt-image 形态)也合法
  const auto l_aspect_only = nlohmann::json{{"model", "gpt-image-2-vip"}, {"prompt", "x"}, {"aspectRatio", "2048x2048"}};
  BOOST_REQUIRE_NO_THROW(l_aspect_only.get<transfer_station_request_t>());
}

// 选填键仍然是宽容的: 认得出的键类型不对就不读(跳过, 不是猜), 且永不抛
// 注意这里没有「取值写错必须抛」这一条: 四个字段是字符串, 取值域在上游,
// 本工程只认字符串形态, 取值一律照发(见 image_request.h 的「为什么这里没有枚举」)
BOOST_AUTO_TEST_CASE(from_json_skips_wrongly_typed_optional_keys) {
  const auto l_not_string = nlohmann::json{
      {"model", "nano-banana-2"}, {"prompt", "x"}, {"imageSize", "1K"}, {"quality", 2},
      {"background", true},       {"replyType", nullptr}
  };
  transfer_station_request_t l_req{};
  BOOST_REQUIRE_NO_THROW(l_req = l_not_string.get<transfer_station_request_t>());
  BOOST_CHECK_EQUAL(l_req.quality_, "");
  BOOST_CHECK_EQUAL(l_req.background_, "");
  BOOST_CHECK_EQUAL(l_req.reply_type_, "");

  // 取值本身不校验: 上游没答应的取值也照读照发, 错在别处报
  const auto l_unknown = nlohmann::json{{"model", "nano-banana-2"}, {"prompt", "x"}, {"imageSize", "3K"}};
  BOOST_REQUIRE_NO_THROW(l_req = l_unknown.get<transfer_station_request_t>());
  BOOST_CHECK_EQUAL(l_req.image_size_, "3K");
  BOOST_CHECK_EQUAL(nlohmann::json{l_req}.at("imageSize").get<std::string>(), "3K");
}

// 完整往返: 发出去的 json 读回来必须与原来等价
BOOST_AUTO_TEST_CASE(full_round_trip) {
  const auto l_src = gpt_image_request();
  const auto l_back = nlohmann::json{l_src}.get<transfer_station_request_t>();

  BOOST_CHECK_EQUAL(l_back.model_, l_src.model_);
  BOOST_CHECK_EQUAL(l_back.prompt_, l_src.prompt_);
  BOOST_CHECK_EQUAL(l_back.aspect_ratio_, l_src.aspect_ratio_);
  BOOST_CHECK_EQUAL(l_back.mask_, l_src.mask_);
  BOOST_CHECK_EQUAL(l_back.quality_, l_src.quality_);
  BOOST_CHECK_EQUAL(l_back.image_size_, "");
  BOOST_CHECK_EQUAL(l_back.background_, "");
  BOOST_CHECK_EQUAL(l_back.reply_type_, "");
  // 再序列化一次必须逐字相等(选了会被反复覆盖的 replyType 之后, 这是最难发现漂移的一处)
  BOOST_CHECK_EQUAL(nlohmann::json{l_back}.dump(), nlohmann::json{l_src}.dump());
}

// 单值字段(background)的往返: 有值读得回来
BOOST_AUTO_TEST_CASE(single_value_enum_round_trip) {
  transfer_station_request_t l_req{};
  l_req.model_        = "gpt-image-2.5-sunburst";
  l_req.prompt_       = "x";
  l_req.aspect_ratio_ = "2048x2048";
  l_req.background_   = "transparent";
  l_req.quality_      = "max";

  const auto l_back = nlohmann::json{l_req}.get<transfer_station_request_t>();
  BOOST_CHECK_EQUAL(l_back.background_, "transparent");
  BOOST_CHECK_EQUAL(l_back.quality_, "max");
}

// null 的选填键 = 未设置, 不是错误; 也不是「把字段读成字符串 "null"」
BOOST_AUTO_TEST_CASE(null_optional_keys_read_as_unset) {
  const auto l_null = nlohmann::json{
      {"model", "nano-banana-2"}, {"prompt", "x"}, {"imageSize", "1K"}, {"quality", nullptr}
  };
  transfer_station_request_t l_req{};
  BOOST_REQUIRE_NO_THROW(l_req = l_null.get<transfer_station_request_t>());
  BOOST_CHECK_EQUAL(l_req.quality_, "");
  // 也确认它不会被当成非空而发出去
  BOOST_CHECK_EQUAL(nlohmann::json{l_req}.contains("quality"), false);
}

// 请求体里不得出现任何凭据: 密钥走 Authorization 头, 不进 body
BOOST_AUTO_TEST_CASE(request_body_carries_no_credentials) {
  const auto l_dump = nlohmann::json{nano_banana_request()}.dump();
  BOOST_CHECK_EQUAL(l_dump.find("sk-"), std::string::npos);
  BOOST_CHECK_EQUAL(l_dump.find("Bearer"), std::string::npos);
  BOOST_CHECK_EQUAL(l_dump.find("token"), std::string::npos);
  BOOST_CHECK_EQUAL(l_dump.find("key"), std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
