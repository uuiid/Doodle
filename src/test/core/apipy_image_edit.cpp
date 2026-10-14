//
// API易 图片客户端 (apipy_image_client) 的**请求体建模与改图路径**测试
//
// 盯的是三件事:
//   1) 请求体走 transfer_station_request_t(与中转站共用一份类型)之后, 键名映射必须正确 ——
//      两边只差一处同义键(中转站 aspectRatio / API易 size), 外加参考图 images[] -> content[];
//      映射错了上游只会回一句 "generate failed", 本地怎么跑都是通的;
//   2) 分流判据是请求体里有没有参考图 —— 判错会走错端点, 而 generations 不接受
//      multipart 体, 现场表现是 400(且 upstream 不会告诉你"你走错端点了");
//   3) edits **不接受 URL**, 参考图必须编成文件负担(官方文档明说 image 是 file 类型),
//      所以「URL 有没有被正确抽出来并变成负担」是这条路的正确性核心。
//
// 全部是纯函数, 不联网、不读盘、不需要 app_base。
//

#include <doodle_core/metadata/seedance2/image_request.h>

#include <doodle_lib/http_client/apipy_image_client.h>
#include <doodle_core/exception/exception.h>

#include <boost/test/unit_test.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <string>
#include <string_view>
#include <vector>

BOOST_AUTO_TEST_SUITE(apipy_image_edit)

namespace {

using namespace doodle;
using namespace doodle::http::seedance2;
using doodle::seedance2::transfer_station_request_t;

/// 站点内参考图的固定 URL(改图路径的输入)
constexpr std::string_view g_site_image{"/api/seedance2/pictures/c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d.png"};

/// 造一份「一张参考图」的 API易 原生形状请求体(前端 PUT 进来的就是这一份)
nlohmann::json apipy_shape_request() {
  return nlohmann::json{
      {"model", "gpt-image-2.5-vip"},
      {"prompt", "把背景换成海边黄昏"},
      {"size", "2048x1152"},
      {"content",
       nlohmann::json::array(
           {nlohmann::json{{"type", "image_url"}, {"image_url", nlohmann::json{{"url", std::string{g_site_image}}}}}}
       )}
  };
}

/// 纯文生图的 API易 原生形状(没有 content)
nlohmann::json text_to_image_request() {
  return nlohmann::json{{"model", "gpt-image-2.5-vip"}, {"prompt", "一只猫"}, {"size", "1024x1024"}};
}

}  // namespace

// ─── 类型化读法: API易 原生形状 -> 结构体 ───────────────────────────────────

/// size 是 API易 的叫法, 结构体里落在 aspect_ratio_; content[] 的 URL 顺序原样进 reference_images_
BOOST_AUTO_TEST_CASE(parse_request_reads_apiyi_shape) {
  const auto l_req = apipy_image_client::parse_request(apipy_shape_request());

  BOOST_CHECK_EQUAL(l_req.model_, "gpt-image-2.5-vip");
  BOOST_CHECK_EQUAL(l_req.prompt_, "把背景换成海边黄昏");
  // 同值改键: size 的内容原样落在 aspect_ratio_ 上
  BOOST_CHECK_EQUAL(l_req.aspect_ratio_, "2048x1152");
  BOOST_REQUIRE_EQUAL(l_req.reference_images_.size(), 1);
  BOOST_CHECK_EQUAL(l_req.reference_images_.front(), g_site_image);
  BOOST_CHECK(l_req.has_reference_images());
}

/// 没给 model = 用本后端的默认模型, 不是"缺必填"而抛
BOOST_AUTO_TEST_CASE(parse_request_fills_the_default_model) {
  const auto l_req = apipy_image_client::parse_request(
      nlohmann::json{{"prompt", "一只猫"}, {"size", "1024x1024"}}
  );

  BOOST_CHECK_EQUAL(l_req.model_, apipy_image_client::g_default_model);
}

/// 缺 prompt / 缺尺寸这一类"结构不完整"要当场抛: 发出去必然失败, 而错在本地还能看出是哪个键
BOOST_AUTO_TEST_CASE(parse_request_rejects_incomplete_bodies) {
  // 缺 prompt
  BOOST_CHECK_THROW(
      apipy_image_client::parse_request(nlohmann::json{{"model", "x"}, {"size", "1024x1024"}}),
      nlohmann::json::type_error
  );
  // prompt 与尺寸都在, 但尺寸既不是 size 也不是 aspectRatio
  BOOST_CHECK_THROW(
      apipy_image_client::parse_request(nlohmann::json{{"model", "x"}, {"prompt", "y"}}),
      nlohmann::json::type_error
  );
  // 非对象(这列历史上可能存过 null / 数组)
  BOOST_CHECK_THROW(apipy_image_client::parse_request(nlohmann::json{}), nlohmann::json::type_error);
}

/// content[] 里只有 image_url 算参考图: video_url / audio_url 必须被跳过,
/// 否则会把视频字节当成图传给一个只吃图片的端点
BOOST_AUTO_TEST_CASE(normalize_skips_video_and_audio) {
  const auto l_body = nlohmann::json{
      {"model", "gpt-image-2.5-vip"},
      {"prompt", "看图"},
      {"size", "1024x1024"},
      {"content",
       nlohmann::json::array({
           nlohmann::json{{"video_url", nlohmann::json{{"url", "/api/seedance2/pictures/aaaaaaaa-0000-0000-0000-00000000000a.mp4"}}}},
           nlohmann::json{{"audio_url", nlohmann::json{{"url", "/api/seedance2/pictures/aaaaaaaa-0000-0000-0000-00000000000b.mp3"}}}},
           nlohmann::json{{"image_url", nlohmann::json{{"url", "/api/seedance2/pictures/aaaaaaaa-0000-0000-0000-00000000000c.png"}}}},
       })}
  };

  const auto l_req = apipy_image_client::parse_request(l_body);

  BOOST_REQUIRE_EQUAL(l_req.reference_images_.size(), 1);
  BOOST_CHECK(l_req.reference_images_.front().find("00000000000c") != std::string::npos);
}

/// 多张参考图的**顺序**就是 prompt 里「图1/图2/图3」的指代依据, 不能排序或去重
BOOST_AUTO_TEST_CASE(parse_request_keeps_reference_order) {
  const auto l_body = nlohmann::json{
      {"model", "gpt-image-2.5-vip"},
      {"prompt", "把图1的人物放进图2的场景"},
      {"size", "1024x1024"},
      {"content",
       nlohmann::json::array({
           nlohmann::json{{"image_url", nlohmann::json{{"url", "/api/seedance2/pictures/aaaaaaaa-0000-0000-0000-000000000001.png"}}}},
           nlohmann::json{{"image_url", nlohmann::json{{"url", "/api/seedance2/pictures/aaaaaaaa-0000-0000-0000-000000000002.png"}}}},
           nlohmann::json{{"image_url", nlohmann::json{{"url", "/api/seedance2/pictures/aaaaaaaa-0000-0000-0000-000000000003.png"}}}},
       })}
  };

  const auto l_req = apipy_image_client::parse_request(l_body);

  BOOST_REQUIRE_EQUAL(l_req.reference_images_.size(), 3);
  BOOST_CHECK(l_req.reference_images_.at(0).find("000000000001") != std::string::npos);
  BOOST_CHECK(l_req.reference_images_.at(1).find("000000000002") != std::string::npos);
  BOOST_CHECK(l_req.reference_images_.at(2).find("000000000003") != std::string::npos);
}

/// Responses 形状(input[].content[].image_url 是**字符串**)本后端不认 —— 归一后 input 被摘掉,
/// 于是它既不该漏出参考图, 也不该留下一个漂到上游的信封键
BOOST_AUTO_TEST_CASE(normalize_drops_the_responses_envelope) {
  const auto l_body = nlohmann::json{
      {"model", "gpt-image-2.5-vip"},
      {"prompt", "融合这两张图"},
      {"size", "1024x1024"},
      {"input",
       nlohmann::json::array({
           nlohmann::json{
               {"role", "user"},
               {"content", nlohmann::json::array({
                   nlohmann::json{{"type", "input_image"}, {"image_url", std::string{g_site_image}}},
               })}
           },
       })}
  };

  const auto l_norm = apipy_image_client::normalize_apipy_request(l_body);

  BOOST_CHECK_EQUAL(l_norm.contains("input"), false);
  BOOST_CHECK_EQUAL(l_norm.contains("content"), false);
  // API易 只认 content[] 这一种参考图形状, 所以这里抽不出参考图 -> 走文生图那条路
  BOOST_CHECK(apipy_image_client::parse_request(l_body).reference_images_.empty());
}

// ─── 结构体 -> API易 JSON 的键名映射 ────────────────────────────────────────

/// 分类键映射: aspectRatio -> size, images[] -> content[].image_url.url,
/// 中转站独有的 mask/replyType/background 不出现在 API易 的请求体里
BOOST_AUTO_TEST_CASE(to_apipy_json_renames_the_divergent_keys) {
  transfer_station_request_t l_req{};
  l_req.model_            = "gpt-image-2.5-vip";
  l_req.prompt_           = "把背景换成纯白";
  l_req.aspect_ratio_     = "2048x1152";
  l_req.reference_images_ = std::vector<std::string>{std::string{g_site_image}};
  l_req.quality_          = "high";
  l_req.mask_             = std::string{g_site_image};
  l_req.reply_type_       = "async";
  l_req.background_       = "transparent";

  const auto l_out = apipy_image_client::to_apipy_json(l_req);

  BOOST_CHECK_EQUAL(l_out.contains("aspectRatio"), false);
  BOOST_CHECK_EQUAL(l_out.at("size").get<std::string>(), "2048x1152");
  BOOST_CHECK_EQUAL(l_out.contains("images"), false);
  BOOST_REQUIRE_EQUAL(l_out.at("content").size(), 1U);
  BOOST_CHECK_EQUAL(l_out.at("content").at(0).at("image_url").at("url").get<std::string>(), g_site_image);
  BOOST_CHECK_EQUAL(l_out.at("content").at(0).at("type").get<std::string>(), "image_url");
  // 逐字保留的标量
  BOOST_CHECK_EQUAL(l_out.at("model").get<std::string>(), "gpt-image-2.5-vip");
  BOOST_CHECK_EQUAL(l_out.at("prompt").get<std::string>(), "把背景换成纯白");
  BOOST_CHECK_EQUAL(l_out.at("quality").get<std::string>(), "high");
  // 中转站口径的键不进 API易 的请求体
  BOOST_CHECK_EQUAL(l_out.contains("mask"), false);
  BOOST_CHECK_EQUAL(l_out.contains("replyType"), false);
  BOOST_CHECK_EQUAL(l_out.contains("background"), false);
  // 两条路都显式要 url 输出(b64_json 是数 MB 的字符串, 会撑大 sqlite 行)
  BOOST_CHECK_EQUAL(l_out.at("response_format").get<std::string>(), "url");
}

/// 没有参考图时, 请求体里**不能**出现空 content: 「未设置」与「设成空」是两回事
BOOST_AUTO_TEST_CASE(to_apipy_json_omits_content_without_references) {
  transfer_station_request_t l_req{};
  l_req.model_        = "gpt-image-2.5-vip";
  l_req.prompt_       = "一只猫";
  l_req.aspect_ratio_ = "1024x1024";

  const auto l_out = apipy_image_client::to_apipy_json(l_req);

  BOOST_CHECK_EQUAL(l_out.contains("content"), false);
  BOOST_CHECK_EQUAL(l_out.at("size").get<std::string>(), "1024x1024");
  BOOST_CHECK_EQUAL(l_out.at("response_format").get<std::string>(), "url");
}

/// imageSize(nano-banana 的叫法)是前端可能提交的第三种尺寸键: 归一后它就是 size,
/// 而不是一个上游会忽略的残留键
BOOST_AUTO_TEST_CASE(to_apipy_json_maps_image_size_to_size) {
  transfer_station_request_t l_req{};
  l_req.model_      = "gpt-image-2.5-vip";
  l_req.prompt_     = "x";
  l_req.image_size_ = "1K";

  const auto l_out = apipy_image_client::to_apipy_json(l_req);

  BOOST_CHECK_EQUAL(l_out.contains("imageSize"), false);
  BOOST_CHECK_EQUAL(l_out.at("size").get<std::string>(), "1K");
}

/// 完整往返: API易 原生形状 -> 结构体 -> API易 JSON, 键名与取值都不漂移
BOOST_AUTO_TEST_CASE(normalize_then_to_json_round_trips) {
  const auto l_req = apipy_image_client::parse_request(apipy_shape_request());
  const auto l_out = apipy_image_client::to_apipy_json(l_req);

  BOOST_CHECK_EQUAL(l_out.at("model").get<std::string>(), "gpt-image-2.5-vip");
  BOOST_CHECK_EQUAL(l_out.at("prompt").get<std::string>(), "把背景换成海边黄昏");
  BOOST_CHECK_EQUAL(l_out.at("size").get<std::string>(), "2048x1152");
  BOOST_CHECK_EQUAL(l_out.at("content").at(0).at("image_url").at("url").get<std::string>(), g_site_image);
}

/// 纯文生图: 抽出的参考图为空 —— 空才走 generations
BOOST_AUTO_TEST_CASE(text_to_image_has_no_references) {
  BOOST_CHECK(apipy_image_client::parse_request(text_to_image_request()).reference_images_.empty());
}

// ─── multipart 编码 ─────────────────────────────────────────────────────────

/// 参考图必须编成**文件负担**: 带 filename 与 Content-Type, 且用同名 image 字段
BOOST_AUTO_TEST_CASE(multipart_encodes_reference_as_file_part) {
  std::vector<apipy_image_client::edit_reference_t> l_refs{};
  l_refs.push_back(
      apipy_image_client::edit_reference_t{
          .url_ = "/api/seedance2/pictures/aaaaaaaa-0000-0000-0000-00000000000c.png",
          .filename_ = "aaaaaaaa-0000-0000-0000-00000000000c.png",
          .mime_ = "image/png",
          .bytes_ = "PNGDATA"
      }
  );

  auto l_form = apipy_image_client::build_edit_multipart(apipy_shape_request(), l_refs);

  // filename + Content-Type 都在, 否则上游会当成普通文本字段
  BOOST_CHECK(l_form.body_.find("filename=\"aaaaaaaa-0000-0000-0000-00000000000c.png\"") != std::string::npos);
  BOOST_CHECK(l_form.body_.find("Content-Type: image/png") != std::string::npos);
  // 字段名是 image(官方要求同名可重复 = 数组语义)
  BOOST_CHECK(
      l_form.body_.find("name=\"image\"") != std::string::npos
  );
  // 图片字节原样在里面
  BOOST_CHECK(l_form.body_.find("PNGDATA") != std::string::npos);
  // boundary 同时出现在 Content-Type 与体里(不一致是最典型的翻车)
  BOOST_CHECK(l_form.body_.find("--" + l_form.boundary_ + "\r\n") != std::string::npos);
  BOOST_CHECK(
      l_form.content_type() == "multipart/form-data; boundary=" + l_form.boundary_
  );
}

/// 多张参考图 => 多个**同名** image 负担(数组语义), 顺序保持
BOOST_AUTO_TEST_CASE(multipart_repeats_image_field) {
  std::vector<apipy_image_client::edit_reference_t> l_refs{};
  for (const auto* l_tag : {"AAA", "BBB", "CCC"})
    l_refs.push_back(
        apipy_image_client::edit_reference_t{.url_ = "u", .filename_ = "f.png", .mime_ = "image/png", .bytes_ = l_tag}
    );

  auto l_form = apipy_image_client::build_edit_multipart(nlohmann::json{{"prompt", "融合"}}, l_refs);

  // 出现三次 name="image"
  std::size_t l_count = 0;
  for (auto l_pos = l_form.body_.find("name=\"image\""); l_pos != std::string::npos;
       l_pos      = l_form.body_.find("name=\"image\"", l_pos + 1))
    ++l_count;
  BOOST_CHECK_EQUAL(l_count, 3);

  // 顺序即上传顺序: AAA 在 BBB 前, BBB 在 CCC 前
  BOOST_CHECK(l_form.body_.find("AAA") < l_form.body_.find("BBB"));
  BOOST_CHECK(l_form.body_.find("BBB") < l_form.body_.find("CCC"));
}

/// 标量文本字段要进表单, 但素材字段必须被排除
/// 这里的输入就是 submit_task 真正会传的那一份(结构体 -> to_apipy_json 之后的 API易 形状),
/// 所以它同时钉住了「映射出来的键名原样进表单」这件事
BOOST_AUTO_TEST_CASE(multipart_text_fields_and_exclusions) {
  transfer_station_request_t l_req{};
  l_req.model_            = "gpt-image-2.5-vip";
  l_req.prompt_           = "把背景换成海边黄昏";
  l_req.aspect_ratio_     = "2048x1152";
  l_req.quality_          = "high";
  l_req.reference_images_ = std::vector<std::string>{std::string{g_site_image}};
  auto l_body             = apipy_image_client::to_apipy_json(l_req);
  // 前端塞进来的多余标量: 本客户端不拦它, 由上游按 400 拒绝
  l_body["n"] = 1;

  auto l_form = apipy_image_client::build_edit_multipart(l_body, {});

  BOOST_CHECK(l_form.body_.find("name=\"model\"") != std::string::npos);
  BOOST_CHECK(l_form.body_.find("name=\"prompt\"") != std::string::npos);
  // size 是映射后的键名(不是 aspectRatio), response_format 也照旧在
  BOOST_CHECK(l_form.body_.find("name=\"size\"") != std::string::npos);
  BOOST_CHECK(l_form.body_.find("name=\"aspectRatio\"") == std::string::npos);
  BOOST_CHECK(l_form.body_.find("name=\"quality\"") != std::string::npos);
  BOOST_CHECK(l_form.body_.find("name=\"response_format\"") != std::string::npos);
  BOOST_CHECK(l_form.body_.find("name=\"n\"") != std::string::npos);
  // 素材字段不进表单(它们已经变成文件负担), 否则上游会拿到一个它不认的 JSON 串
  BOOST_CHECK(l_form.body_.find("name=\"content\"") == std::string::npos);
  BOOST_CHECK(l_form.body_.find("name=\"images\"") == std::string::npos);
  BOOST_CHECK(l_form.body_.find("name=\"input\"") == std::string::npos);
  // 上面那份请求体是带参考图的, 但这里没给文件负担 —— 确认 URL 不会以文本形式漏进去
  BOOST_CHECK(l_form.body_.find(g_site_image) == std::string::npos);
}

/// 收尾 boundary: 体必须以 "--<boundary>--\r\n" 结束, 少了它上游会一直等下一个部分
BOOST_AUTO_TEST_CASE(multipart_ends_with_closing_boundary) {
  auto l_form = apipy_image_client::build_edit_multipart(nlohmann::json{{"prompt", "x"}}, {});

  const auto l_expected = "--" + l_form.boundary_ + "--\r\n";
  BOOST_REQUIRE(l_form.body_.size() >= l_expected.size());
  BOOST_CHECK_EQUAL(l_form.body_.substr(l_form.body_.size() - l_expected.size()), l_expected);
}

/// 字段值里的换行必须被清掉, 否则会把 multipart 的头部结构截断
BOOST_AUTO_TEST_CASE(multipart_strips_crlf_from_filename) {
  std::vector<apipy_image_client::edit_reference_t> l_refs{};
  l_refs.push_back(
      apipy_image_client::edit_reference_t{
          .url_ = "u", .filename_ = "bad\r\nname.png", .mime_ = "image/png", .bytes_ = "X"
      }
  );

  auto l_form = apipy_image_client::build_edit_multipart(nlohmann::json{{"prompt", "x"}}, l_refs);

  // 不应出现 "bad\r\nname.png" 这种被注入的换行
  BOOST_CHECK(l_form.body_.find("bad\r\nname.png") == std::string::npos);
  BOOST_CHECK(l_form.body_.find("badname.png") != std::string::npos);
}

// ─── MIME 推断 ─────────────────────────────────────────────────────────────

/// 官方只接受 png/jpg/webp; 认不出的扩展名按 png(硬报别的只会让上游拒得更莫名其妙)
BOOST_AUTO_TEST_CASE(mime_of_image_mapping) {
  BOOST_CHECK_EQUAL(apipy_image_client::mime_of_image(".png"), "image/png");
  BOOST_CHECK_EQUAL(apipy_image_client::mime_of_image(".jpg"), "image/jpeg");
  BOOST_CHECK_EQUAL(apipy_image_client::mime_of_image(".jpeg"), "image/jpeg");
  BOOST_CHECK_EQUAL(apipy_image_client::mime_of_image(".webp"), "image/webp");
  // 大小写不敏感(URL 里 .PNG 很常见)
  BOOST_CHECK_EQUAL(apipy_image_client::mime_of_image(".PNG"), "image/png");
  BOOST_CHECK_EQUAL(apipy_image_client::mime_of_image(".JPG"), "image/jpeg");
  // 未知/空 -> png
  BOOST_CHECK_EQUAL(apipy_image_client::mime_of_image(".gif"), "image/png");
  BOOST_CHECK_EQUAL(apipy_image_client::mime_of_image(""), "image/png");
}

// ─── 站点内图片 URL -> 本地文件路径 ──────────────────────────────────────────

/// 本站图片路径要能拆出 uuid 与扩展名; 带签名查询串也要认
BOOST_AUTO_TEST_CASE(parse_site_image_id_uuid_and_ext) {
  auto l_parsed = apipy_image_client::parse_site_image_id(
      "/api/seedance2/pictures/c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d.png"
  );

  BOOST_REQUIRE(l_parsed.has_value());
  BOOST_CHECK_EQUAL(l_parsed->extension_, ".png");
  BOOST_CHECK_EQUAL(
      boost::uuids::to_string(l_parsed->id_), "c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d"
  );

  // 带签名查询串(exp/uid/sig)也要认出来 —— 前端可能把签名后的 URL 原样传回来
  auto l_signed = apipy_image_client::parse_site_image_id(
      "/api/seedance2/pictures/c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d.png"
      "?exp=1770000000&uid=c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d&sig=deadbeef"
  );
  BOOST_REQUIRE(l_signed.has_value());
  BOOST_CHECK_EQUAL(l_signed->extension_, ".png");
  BOOST_CHECK_EQUAL(boost::uuids::to_string(l_signed->id_), boost::uuids::to_string(l_parsed->id_));

  // 扩展名允许缺省(路由里的 file_extension_ 可省)
  auto l_no_ext =
      apipy_image_client::parse_site_image_id("/api/seedance2/pictures/c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d");
  BOOST_REQUIRE(l_no_ext.has_value());
  BOOST_CHECK(l_no_ext->extension_.empty());
}

/// 绝对 URL / data: / 别的路径都不是本站图片, 必须返回 nullopt(交给调用方走别的分支)
BOOST_AUTO_TEST_CASE(parse_site_image_id_rejects_non_site) {
  BOOST_CHECK(!apipy_image_client::parse_site_image_id("https://example.com/a.png").has_value());
  BOOST_CHECK(
      !apipy_image_client::parse_site_image_id(
           "http://1.2.3.4:80/api/seedance2/pictures/c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d.png"
      )
          .has_value()
  );
  BOOST_CHECK(!apipy_image_client::parse_site_image_id("data:image/png;base64,AAAA").has_value());
  BOOST_CHECK(!apipy_image_client::parse_site_image_id("").has_value());
  // 前缀对但不是本站图片路由
  BOOST_CHECK(!apipy_image_client::parse_site_image_id("/api/seedance2/thumbnail/x.png").has_value());
  // 前缀对但 id 不是 uuid
  BOOST_CHECK(!apipy_image_client::parse_site_image_id("/api/seedance2/pictures/not-a-uuid.png").has_value());
  // 前缀对但路径为空
  BOOST_CHECK(!apipy_image_client::parse_site_image_id("/api/seedance2/pictures/").has_value());
}

// ─── 参考图 URI 硬检查 ───────────────────────────────────────────────────────

/// 合法: 只有 reg.h 的 seedance2_pictures 那一条形状, 返回值原样是 URL
BOOST_AUTO_TEST_CASE(checked_reference_url_accepts_site_pictures_route) {
  BOOST_CHECK_EQUAL(apipy_image_client::checked_reference_url(g_site_image), g_site_image);

  // 带签名查询串 / 缺扩展名 也算合法形状(路由的 file_extension_ 可省)
  constexpr std::string_view l_signed{
      "/api/seedance2/pictures/c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d.png?exp=1770000000&uid=u&sig=s"
  };
  BOOST_CHECK_EQUAL(apipy_image_client::checked_reference_url(l_signed), l_signed);
  BOOST_CHECK_EQUAL(
      apipy_image_client::checked_reference_url("/api/seedance2/pictures/c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d"),
      "/api/seedance2/pictures/c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d"
  );
}

/// 不合法的一律 400。重点是**绝对 URL 与 data:** 也被拒 —— 它们过去是两条取图旁路,
/// 放行等于让本后端替调用方取任意网址(SSRF), 而它的职责只是读本站素材。
BOOST_AUTO_TEST_CASE(checked_reference_url_rejects_everything_else) {
  for (const auto* l_bad : {
           // 绝对 URL: 曾是"下载退路", 现在必须拒
           "https://example.com/a.png",
           "http://1.2.3.4:80/api/seedance2/pictures/c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d.png",
           // 内联 base64: 曾是"前端内联"退路, 现在必须拒
           "data:image/png;base64,AAAA",
           "blob:http://localhost/abc",
           // 本站别的路由 / 形状不对
           "/api/seedance2/thumbnail/x.png",
           "/api/seedance2/pictures/not-a-uuid.png",
           "/api/seedance2/pictures/",
           "",
       }) {
    // 返回值是 [[nodiscard]]; 这里只关心它抛不抛, 所以显式丢掉结果。
    BOOST_CHECK_THROW(static_cast<void>(apipy_image_client::checked_reference_url(l_bad)), http_request_error);
  }
}

BOOST_AUTO_TEST_SUITE_END()
