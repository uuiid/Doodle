#include "apipy_image_client.h"

#include "doodle_core/exception/exception.h"
#include <doodle_core/metadata/kitsu_ctx_t.h>

#include <doodle_lib/core/base64.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/lib_warp/boost_fmt_beast.h>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "core/global_function.h"
#include <array>
#include <cctype>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <iterator>
#include <ranges>
#include <string>

namespace doodle::http::seedance2 {

namespace {

/// 生成接口路径(文生图)
constexpr std::string_view g_generations_path{"/v1/images/generations"};
/// 图片编辑接口路径(参考图改图 / 多图融合)。
/// 与生成接口的差别不只是路径: edits 是 multipart/form-data, 参考图必须作为**文件负担**
/// 上传, 不接受 URL —— 所以本客户端要把站点内图片先读成本地字节再编进去。
/// 官方文档: https://docs.apiyi.com/api-capabilities/gpt-image-2-vip/image-edit
constexpr std::string_view g_edits_path{apipy_image_client::g_edits_path};

/// b64_json 落盘时用的扩展名。模型固定输出 PNG。
constexpr std::string_view g_default_ext{".png"};

/// 站点内图片路由的前缀(见 http_method/seedance2/reg.h 的 seedance2_pictures)
constexpr std::string_view g_pictures_prefix{"/api/seedance2/pictures/"};

/// 生成一个 multipart boundary。必须**不出现在任何负担的内容里**, 所以用随机 uuid:
/// 图片字节里恰好含有这个 36 字符十六进制串的概率可以忽略。
std::string make_boundary() { return "----doodle" + boost::uuids::to_string(core_set::get_set().get_uuid()); }

/// 把一段文本里的换行去掉。multipart 的头部是逐行 CRLF 分隔的, 值里混进 \n 会把头部截断
/// (mime 冲突); filename 也可能来自 URL, 同理。
std::string strip_crlf(std::string_view in_text) {
  std::string l_out{};
  l_out.reserve(in_text.size());
  for (const auto l_c : in_text)
    if (l_c != '\r' && l_c != '\n') l_out.push_back(l_c);
  return l_out;
}

/// 顶层标量字段(除素材字段外)编成一个文本负担。
/// 只收标量: 嵌套对象/数组在 multipart 里没有公认的编码方式, 前端给它们也没用
/// (edits 只认 model/prompt/size/quality/response_format 这几个平铺字段)。
void append_text_part(
    std::string& out, std::string_view in_boundary, std::string_view in_name, std::string_view in_value
) {
  out += "--";
  out += in_boundary;
  out += "\r\nContent-Disposition: form-data; name=\"";
  out += in_name;
  out += "\"\r\n\r\n";
  out += in_value;
  out += "\r\n";
}

/// 一个文件负担
void append_file_part(
    std::string& out, std::string_view in_boundary, std::string_view in_name, std::string_view in_filename,
    std::string_view in_mime, std::string_view in_bytes
) {
  out += "--";
  out += in_boundary;
  out += "\r\nContent-Disposition: form-data; name=\"";
  out += in_name;
  out += "\"; filename=\"";
  out += in_filename;
  out += "\"\r\nContent-Type: ";
  out += in_mime;
  out += "\r\n\r\n";
  out += in_bytes;
  out += "\r\n";
}

/// 响应里 data 数组的第一项; 没有则返回空对象
const nlohmann::json& first_data_item(const nlohmann::json& in_body) {
  static const nlohmann::json l_empty = nlohmann::json::object();
  if (!in_body.is_object()) return l_empty;
  if (!in_body.contains("data")) return l_empty;
  const auto& l_data = in_body.at("data");
  if (!l_data.is_array() || l_data.empty()) return l_empty;
  const auto& l_item = l_data.front();
  return l_item.is_object() ? l_item : l_empty;
}

/// 从 content[] 这一项里取图片 URL; 不是图片(缺 image_url / 空串)时返回 nullopt。
/// **只认 image_url**: 生成 API 是 {"image_url": {"url": "..."}}, Responses 是
/// {"image_url": "..."}; video_url / audio_url 一律跳过 —— edits 只吃参考图,
/// 把视频字节当图传过去上游只会给一个看不懂的报错。
std::optional<std::string> image_url_of_content_item(const nlohmann::json& in_item) {
  if (!in_item.is_object() || !in_item.contains("image_url")) return std::nullopt;
  const auto& l_holder = in_item.at("image_url");
  if (l_holder.is_object() && l_holder.contains("url") && l_holder.at("url").is_string())
    return l_holder.at("url").get<std::string>();
  if (l_holder.is_string()) return l_holder.get<std::string>();
  return std::nullopt;
}

}  // namespace

// ─── 纯函数(便于不联网单测) ──────────────────────────────────────────────────

std::vector<std::string> apipy_image_client::extract_image_urls(const nlohmann::json& in_body) {
  std::vector<std::string> l_urls{};
  const auto& l_item = first_data_item(in_body);
  if (l_item.contains("url") && l_item.at("url").is_string()) {
    auto l_url = l_item.at("url").get<std::string>();
    if (!l_url.empty()) l_urls.push_back(std::move(l_url));
  }
  return l_urls;
}

std::string apipy_image_client::extract_b64_payload(const nlohmann::json& in_body) {
  const auto& l_item = first_data_item(in_body);
  if (!l_item.contains("b64_json") || !l_item.at("b64_json").is_string()) return {};

  auto l_b64 = l_item.at("b64_json").get<std::string>();
  // 官方明确要求做这个检测: 2026-07 实测是**纯 base64**(无前缀), 但历史版本曾直接带
  // "data:image/png;base64," 前缀。两种形态都兼容, 否则会"带前缀解码"产出损坏的图片。
  if (l_b64.starts_with("data:"))
    if (const auto l_comma = l_b64.find(','); l_comma != std::string::npos) l_b64.erase(0, l_comma + 1);
  return l_b64;
}

std::string apipy_image_client::get_error_code(const nlohmann::json& in_body) {
  if (!in_body.is_object() || !in_body.contains("error")) return {};
  const auto& l_err = in_body.at("error");
  if (l_err.is_string()) return l_err.get<std::string>();
  if (!l_err.is_object()) return {};
  // OpenAI 兼容形状: {"error": {"code": ..., "type": ..., "message": ...}}
  for (const auto* l_key : {"code", "type"}) {
    if (l_err.contains(l_key) && l_err.at(l_key).is_string()) return l_err.at(l_key).get<std::string>();
  }
  if (l_err.contains("message") && l_err.at("message").is_string()) return l_err.at("message").get<std::string>();
  return {};
}

std::string apipy_image_client::mime_of_image(const std::string& in_ext) {
  auto l_ext = in_ext;
  std::ranges::transform(l_ext, l_ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (l_ext == ".jpg" || l_ext == ".jpeg") return "image/jpeg";
  if (l_ext == ".webp") return "image/webp";
  // 官方只接受 png/jpg/webp; 其余(含 .png 本身)都按 png —— 认不出的扩展名硬报一个
  // 别的 MIME 只会让上游拒得更莫名其妙。
  return "image/png";
}

FSys::path apipy_image_client::parse_site_image_id(std::string_view in_url) {
  if (!in_url.starts_with(g_pictures_prefix)) return {};

  FSys::path l_name{in_url};
  auto l_ext    = l_name.extension().generic_string();
  auto l_id_str = l_name.stem().generic_string();

  uuid l_id     = from_uuid_str(l_id_str);
  return g_ctx().get<kitsu_ctx_t>().get_sd2_pictures_file(l_id, l_ext);
}

nlohmann::json apipy_image_client::normalize_apipy_request(const nlohmann::json& in_request) {
  if (!in_request.is_object()) return in_request;

  auto l_out = in_request;
  // size -> aspectRatio: 结构体把它俩表达成同一个键的两种取值(比例或像素值),
  // 而 API易 的 generations/edits 用 size 表后者。同值改键, 不改取值。
  if (l_out.contains("size") && !l_out.contains("aspectRatio")) l_out["aspectRatio"] = l_out.at("size");
  l_out.erase("size");

  // content[] -> images[]: 结构体的 reference_images_ 是**字符串数组**, 而 API易 的参考图
  // 是 content[].image_url(.url)。这里只抽 URL, 顺序即图1/图2/图3 的指代顺序, 不排序不去重。
  if (l_out.contains("content") && !l_out.contains("images")) {
    if (const auto& l_content = l_out.at("content"); l_content.is_array()) {
      nlohmann::json l_images = nlohmann::json::array();
      for (const auto& l_item : l_content)
        if (auto l_url = image_url_of_content_item(l_item); l_url && !l_url->empty())
          l_images.push_back(std::move(*l_url));
      if (!l_images.empty()) l_out["images"] = std::move(l_images);
    }
  }
  // 信封字段不再进结构体: Responses 形状的 input[] 本后端不认, 留着只会让它被当成
  // "未知键"漂下去(结构体的 from_json 对未知键宽容跳过, 于是就成了一个字段为空的结构体)。
  l_out.erase("content");
  l_out.erase("input");
  return l_out;
}

apipy_image_client::edits_form_t apipy_image_client::build_edit_multipart(
    const nlohmann::json& in_apipy_body, const std::vector<edit_reference_t>& in_references
) {
  edits_form_t l_form{};
  l_form.boundary_ = make_boundary();

  // 哪些字段不进表单: 素材字段本身(它们已经变成文件负担了)。edits 只认平铺标量,
  // 把 content 塞进去只会被上游当未知字段。
  constexpr std::array g_skipped{"content", "input", "images", "image_url", "video_url", "audio_url"};

  if (in_apipy_body.is_object()) {
    for (const auto& [l_key, l_value] : in_apipy_body.items()) {
      if (!l_value.is_string() && !l_value.is_number() && !l_value.is_boolean()) continue;
      if (std::ranges::find(g_skipped, l_key) != std::ranges::end(g_skipped)) continue;
      append_text_part(
          l_form.body_, l_form.boundary_, l_key, l_value.is_string() ? l_value.get<std::string>() : l_value.dump()
      );
    }
  }

  for (const auto& l_ref : in_references)
    append_file_part(
        l_form.body_, l_form.boundary_, g_edit_image_field, strip_crlf(l_ref.filename_), l_ref.mime_, l_ref.bytes_
    );

  // 收尾 boundary(长度写死: 前导 "--" + boundary + 尾部 "--" + CRLF)
  l_form.body_ += "--";
  l_form.body_ += l_form.boundary_;
  l_form.body_ += "--\r\n";
  return l_form;
}

void to_json(nlohmann::json& j, const apipy_image_client::request_t& req) {
  j["model"]  = req.model_;
  j["prompt"] = req.prompt_;
  j["size"]   = req.size_;
  if (!req.quality_.empty()) j["quality"] = req.quality_;
}

apipy_image_client::edits_form_t apipy_image_client::request_t::to_multipart() const {
  return apipy_image_client::build_edit_multipart(nlohmann::json{*this}, image_bytes_);
}

doodle::seedance2::task_status apipy_image_client::parse_status(const nlohmann::json& in_body, int in_status_code) {
  // 同步接口没有中间态: 要么这次调用带着图回来了, 要么就是失败。
  const bool l_http_ok = in_status_code >= 200 && in_status_code < 300;
  if (!l_http_ok) return doodle::seedance2::task_status::failed;
  if (in_body.contains("error")) return doodle::seedance2::task_status::failed;
  // 有 url 或 b64_json 才算成功。二者只会出现其一(官方说明)。
  if (!extract_image_urls(in_body).empty()) return doodle::seedance2::task_status::succeeded;
  if (!extract_b64_payload(in_body).empty()) return doodle::seedance2::task_status::succeeded;
  return doodle::seedance2::task_status::failed;
}

// ─── 私有工具 ────────────────────────────────────────────────────────────────

FSys::path apipy_image_client::write_b64_to_cache(std::string_view in_b64, std::string_view in_ext) const {
  // 防御性剥前缀: extract_b64_payload 已经剥过一次, 这里兜底, 免得调用方直接把这个
  // 函数用到未处理过的字段上时把 "data:image/png;base64," 当成内容解进去。
  if (in_b64.starts_with("data:"))
    if (const auto l_comma = in_b64.find(','); l_comma != std::string_view::npos) in_b64.remove_prefix(l_comma + 1);

  auto l_path  = core_set::get_set().get_cache_root("http") /
                 (core_set::get_set().get_uuid_str() + std::string{in_ext.empty() ? g_default_ext : in_ext});

  // 用工程里既有的 cpp-base64(见 core/base64.h, socket_io 已用它解二进制帧), 不引新依赖。
  // 解码结果直接落盘, 不再多留一份 —— 4K 图 base64 数 MB, 没必要为它多复制一次。
  auto l_bytes = base64_decode(in_b64);
  DOODLE_CHICK(!l_bytes.empty(), "图片 base64 解码结果为空(长度 {})", in_b64.size());

  std::ofstream l_ofs{l_path, std::ios::binary | std::ios::trunc};
  DOODLE_CHICK(l_ofs.is_open(), "无法写入图片缓存文件: {}", l_path.generic_string());
  l_ofs.write(l_bytes.data(), static_cast<std::streamsize>(l_bytes.size()));
  DOODLE_CHICK(l_ofs.good(), "写入图片缓存失败: {}", l_path.generic_string());
  l_ofs.close();

  SPDLOG_LOGGER_INFO(logger_, "图片已落盘: {} ({} 字节)", l_path.generic_string(), l_bytes.size());
  return l_path;
}

apipy_image_client::edit_reference_t apipy_image_client::load_reference_image(std::string_view in_url) {
  edit_reference_t l_ref{};
  l_ref.url_        = std::string{in_url};
  auto l_local_path = parse_site_image_id(in_url);

  std::ifstream l_ifs{l_local_path, std::ios::binary};
  DOODLE_CHICK(l_ifs.is_open(), "参考图无法读取: {} (URL {})", l_local_path.generic_string(), std::string{in_url});
  l_ref.bytes_    = std::string{std::istreambuf_iterator<char>{l_ifs}, std::istreambuf_iterator<char>{}};
  l_ref.filename_ = l_local_path.filename().generic_string();

  // 扩展名兜底: 文件名可能没有扩展名(比如是 uuid), 那就按 png 报 MIME
  l_ref.mime_     = mime_of_image(l_local_path.extension().generic_string());
  return l_ref;
}

// ─── 子类方法 ────────────────────────────────────────────────────────────────

ai_client_base::request_info_t apipy_image_client::collect_request_info(const nlohmann::json& in_request) const {
  // 与 transfer_station_client 同一条读法: 键名/取值域的判据全部落在
  // transfer_station_request_t 上, 本函数里不出现裸键名。
  const auto l_req = in_request.get<doodle::seedance2::transfer_station_request_t>();

  DOODLE_CHICK_HTTP(
      l_req.reference_images_.size() <= g_max_reference_images, bad_request,
      fmt::format("参考图最多 {} 张, 收到 {} 张", g_max_reference_images, l_req.reference_images_.size())
  );

  for (const auto& url : l_req.reference_images_) {
    // 只有本站图片路由(reg.h 的 seedance2_pictures)能读成本地字节; 绝对 URL / data: 一律拒,
    // 否则本后端会替调用方去取任意网址(SSRF)。
    const auto l_path = parse_site_image_id(url);
    if (l_path.empty() || !FSys::exists(l_path) || FSys::file_size(l_path) == 0)
      throw_exception(
          http_request_error{
              boost::beast::http::status::bad_request,
              fmt::format(
                  "参考图 URL 必须指向本站图片路由 {}<uuid><扩展名> 且文件存在, 收到: {}", g_pictures_prefix,
                  std::string{url}
              )
          }
      );
  }

  request_info_t l_info{};
  l_info.model_       = l_req.model_;
  // 本后端用 size(宽x高, 如 "2048x1152") 表示分辨率, 在结构体里落在 aspect_ratio_
  // (与 gpt-image 在**中转站**那边一样是"像素值放在这个键里"); imageSize 只在前端按
  // nano-banana 叫法提交时才会出现, 作为退路一并认下。
  l_info.resolution_  = l_req.aspect_ratio_.empty() ? l_req.image_size_ : l_req.aspect_ratio_;
  l_info.text_prompt_ = l_req.prompt_;
  return l_info;
}

std::size_t apipy_image_client::default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const {
  switch (in_task_type) {
    case doodle::seedance2::task_type::video:
      // 本后端只做图片, 走到这里说明路由配置错了
      return doodle_config::g_max_task_completion_tokens;
    case doodle::seedance2::task_type::inference:
      // 同理: 本后端不提供推理接口
      return doodle_config::g_ark_inference_default_consumed_tokens;
    default:
      // 图片: 按次计费, 官方是 $0.03/张。这里给一个保守的预扣值 ——
      // 真正计费口径由调用方(接缝)按 token 用量结算, 本值只用于配额检查。
      return doodle_config::g_ark_inference_default_consumed_tokens;
  }
}

boost::asio::awaitable<ai_client_base::run_task_result_t> apipy_image_client::submit_task(
    const doodle::seedance2::task& in_task
) {
  run_task_result_t l_result{};
  l_result.status_ = doodle::seedance2::task_status::failed;

  // 本后端只有同步生成这一条路。推理任务走方舟(seedance2_client), 视频任务走别的后端 ——
  // 走到这里说明路由配置错了。与 transfer_station_client 一样, 在取 IP / 发请求**之前**返回。
  if (in_task.type_ == doodle::seedance2::task_type::inference) {
    logger_->error("API易 图片后端不支持推理任务");
    l_result.data_response_ = nlohmann::json{{"error", "API易 图片后端不支持推理任务"}};
    co_return l_result;
  }
  if (in_task.type_ == doodle::seedance2::task_type::video) {
    logger_->error("API易 图片后端不支持视频任务");
    l_result.data_response_ = nlohmann::json{{"error", "API易 图片后端不支持视频任务"}};
    co_return l_result;
  }

  // ─── 请求体定稿: 类型化读入 -> 本后端改写 -> 再序列化 ───────────────────────
  // 前端把该模型的请求体塞在 data_request_ 里(与中转站共用一份类型), 这里只做三件事:
  //   1) size -> aspectRatio 的语法归一(前端按 API易 原生形状提交, 落库的就是那个形状);
  //   2) 补默认 model;
  //   3) 抽出参考图列表 —— 顺序即 prompt 里「图1/图2/图3」的指代顺序, 不排序不去重。
  // 之后 l_req_body 是**唯一**事实来源: JSON 体与 multipart 都从它派生, 所以两条路上
  // response_format/参考图不可能出现"只改了其中一条"的漂移。
  const auto l_req_body   = in_task.data_request_.get<doodle::seedance2::transfer_station_request_t>();
  const auto l_references = l_req_body.reference_images_;  // 拷贝: 后面要逐张读字节
  apipy_image_client::request_t l_apipy_req{l_req_body};

  boost::beast::http::request<boost::beast::http::string_body> l_req{boost::beast::http::verb::post, {}, 11};
  if (l_references.empty()) {
    auto l_body = (nlohmann::json{} = l_apipy_req);
    l_req.target(std::string{g_generations_path});
    l_req.body() = l_body.dump();
    l_req.set(boost::beast::http::field::content_type, "application/json");
  } else {
    // 逐张读成本地字节; 形状不合法/文件缺失由 collect_request_info 在提交前拦下。
    for (const auto& l_url : l_references) l_apipy_req.image_bytes_.push_back(load_reference_image(l_url));

    // edits 只认平铺标量; 素材 URL 不改写 —— 它们已被抽成文件负担, 上游看不到 URL。
    auto l_form = l_apipy_req.to_multipart();
    l_req.target(std::string{g_edits_path});
    l_req.body() = std::move(l_form.body_);
    l_req.set(boost::beast::http::field::content_type, l_form.content_type());

    SPDLOG_LOGGER_INFO(
        logger_, "API易 图片编辑: {} 张参考图, 请求体 {} 字节", l_apipy_req.image_bytes_.size(), l_req.body().size()
    );
  }

  l_req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token_));
  l_req.set(boost::beast::http::field::accept, "application/json");
  l_req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_);
  l_req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
  // 同步长请求: 必须留足, 见 g_request_timeout 的说明。
  http_client_ptr_->set_timeout(g_request_timeout);

  boost::beast::http::response<http::basic_json_body> l_res{};
  try {
    co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  } catch (...) {
    throw_exception(
        http_request_error{
            boost::beast::http::status::internal_server_error, boost::current_exception_diagnostic_information()
        }
    );
  }

  l_result.data_response_ = l_res.body();
  l_result.status_        = parse_status(l_result.data_response_, static_cast<int>(l_res.result()));
  l_result.error_code_    = get_error_code(l_result.data_response_);

  // token 用量: 同步接口在响应里直接给 usage, 与推理任务同口径, 所以填 run_task_result_t
  // 的 token 字段(而不是等 query_task —— 这个后端没有 query_task)。
  if (l_result.data_response_.contains("usage") && l_result.data_response_.at("usage").is_object()) {
    const auto& l_usage     = l_result.data_response_.at("usage");
    l_result.input_tokens_  = l_usage.value("input_tokens", std::int64_t{0});
    l_result.output_tokens_ = l_usage.value("output_tokens", std::int64_t{0});
    l_result.total_tokens_  = l_usage.value("total_tokens", std::int64_t{0});
  }

  if (l_result.status_ != doodle::seedance2::task_status::succeeded) {
    // 失败原因原样留在 data_response_ 里交给调用方(前端错误一律由响应体描述, 见工程约定)
    SPDLOG_LOGGER_ERROR(
        logger_, "API易 图片生成失败: HTTP {} error_code `{}` body {}", l_res.result(), l_result.error_code_,
        l_result.data_response_.dump()
    );
    co_return l_result;
  }

  // 立刻落盘。这里必须**在本函数内**完成下载, 不能只把临时 URL 交出去:
  // 官方说明 URL 是临时链接(约 24 小时失效), 过期后 404, 而本后端没有 query_task
  // 可以在之后再取一次。download_raw_file 自带 3 次重试与缓存目录落点。
  for (const auto& l_url : extract_image_urls(l_result.data_response_)) {
    l_result.result_files_.push_back(l_url);
    l_result.result_file_paths_.push_back(co_await download_raw_file(l_url));
  }
  if (l_result.result_file_paths_.empty()) {
    // url 模式没给链接: 退回 base64 分支(令牌分组未切 image2_OSS 时会发生)
    const auto l_b64 = extract_b64_payload(l_result.data_response_);
    DOODLE_CHICK(!l_b64.empty(), "API易 图片响应里既没有 url 也没有 b64_json");
    l_result.result_file_paths_.push_back(write_b64_to_cache(l_b64, g_default_ext));
  }

  SPDLOG_LOGGER_INFO(
      logger_, "API易 图片生成完成: {} 个文件, tokens {}/{}", l_result.result_file_paths_.size(),
      l_result.input_tokens_, l_result.output_tokens_
  );
  co_return l_result;
}

// ─── 以下三个是本后端的**空接缝** ───────────────────────────────────────────
// API易 图片 API 是同步单次调用: 没有 task_id、没有查询接口、没有取消接口
// (官方最佳实践页明确写了「没有异步任务 ID、没有轮询接口」)。
// 这三个函数只是为了让子类满足基类的纯虚签名而存在, 在正确接入下**永远不该被调用**:
//   * query_task / cancel_task —— 没有对应接口;
//   * download_result —— 下载发生在本类的 submit_task 内部(见上), 没有别的东西可传。
// 因此它们一律抛错而不是静默返回空结果: 静默返回会让调用方以为"查到了但没结果"。
// 与 transfer_station_client::cancel_task 的处理方式一致。

boost::asio::awaitable<ai_client_base::run_task_result_t> apipy_image_client::query_task(
    const doodle::seedance2::task& in_task
) {
  throw_exception(doodle_error{"apipy_image_client 是同步后端, 任务在 submit_task 里直接结束, 不支持 query_task"});
  co_return run_task_result_t{};
}

boost::asio::awaitable<void> apipy_image_client::cancel_task(const std::string& in_task_id) {
  throw_exception(doodle_error{"apipy_image_client 是同步后端, 没有任务可取消, 不支持 cancel_task"});
  co_return;
}

boost::asio::awaitable<void> apipy_image_client::download_result(run_task_result_t& in_data) {
  throw_exception(doodle_error{"apipy_image_client 的结果文件在 submit_task 里已落盘, 不支持 download_result"});
  co_return;
}

}  // namespace doodle::http::seedance2
