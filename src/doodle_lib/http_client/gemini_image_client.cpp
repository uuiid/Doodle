#include "gemini_image_client.h"

#include "doodle_core/exception/exception.h"
#include <doodle_core/metadata/kitsu_ctx_t.h>

#include <doodle_lib/core/base64.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/lib_warp/boost_fmt_beast.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/scope/scope_exit.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "core/global_function.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <optional>
#include <ranges>
#include <semaphore>
#include <string>
#include <tbb/concurrent_hash_map.h>
#include <utility>

namespace doodle::http::seedance2 {

namespace {

constexpr std::string_view g_interactions_path{"/v1beta/interactions"};
constexpr std::string_view g_default_ext{".png"};
constexpr std::string_view g_pictures_prefix{"/api/seedance2/pictures/"};
constexpr std::chrono::seconds g_request_timeout{360};
constexpr std::string_view g_default_model{"gemini-nano-banana-2.1"};
constexpr std::string_view g_default_mime{"image/png"};
constexpr std::string_view g_response_mime{"image/jpeg"};
constexpr std::string_view g_response_ext{".jpg"};
constexpr std::string_view g_image_response_type{"image"};
constexpr std::string_view g_step_model_output{"model_output"};
constexpr std::string_view g_content_text{"text"};
constexpr std::string_view g_content_image{"image"};
constexpr std::string_view g_payload_placeholder{"<base64 已省略>"};

/// Gemini 官方支持的宽高比。上游对认不出的取值直接 400, 所以在本地先收敛到这张表里。
constexpr std::array g_aspect_ratios{
    std::string_view{"1:1"},  std::string_view{"2:3"},  std::string_view{"3:2"}, std::string_view{"3:4"},
    std::string_view{"4:3"},  std::string_view{"4:5"},  std::string_view{"5:4"}, std::string_view{"9:16"},
    std::string_view{"16:9"}, std::string_view{"21:9"},
};

/// Gemini 3 系只认大写 K 的分辨率串; 512px 只有部分模型支持, 这里不主动下发。
constexpr std::array g_image_sizes{
    std::string_view{"1K"},
    std::string_view{"2K"},
    std::string_view{"4K"},
};

/// generation_config.thinking_level 的合法取值。某一层模型不支持某个级别时由上游自行兜底。
constexpr std::array g_thinking_levels{
    std::string_view{"minimal"},
    std::string_view{"medium"},
    std::string_view{"high"},
};

/// 参考图统一重新编码成 PNG 再发送, 不改尺寸。
constexpr std::string_view g_compressed_ext{".png"};

/// 与 apipy_image_client 同构的并发闸门: 同一个进程里同时最多跑 g_max_inflight 个同步请求,
/// 多出来的任务停在 preparing 等下一轮。map_ 里存已经跑完、等调用方来取的结果。
///
/// 单独一份而不是复用 apipy 的队列: 两家是**不同主机、不同配额**的凭据, 共用一个闸门会让
/// 一家的限流把另一家一起卡住。
class gemini_queue_t {
 public:
  static constexpr std::ptrdiff_t g_max_inflight{10};

  static gemini_queue_t& get() {
    static gemini_queue_t l_instance{};
    return l_instance;
  }
  // 传入状态只有 preparing 和 running
  boost::asio::awaitable<ai_client_base::run_task_result_t> acquire(
      std::shared_ptr<ai_client_base> in_client, const doodle::seedance2::task& in_task
  ) {
    ai_client_base::run_task_result_t l_result{.status_ = doodle::seedance2::task_status::failed};
    if (in_task.status_ == doodle::seedance2::task_status::running) {
      if (auto l_done = take(in_task.uuid_id_)) co_return *std::move(l_done);
      l_result.status_ = doodle::seedance2::task_status::running;
      co_return l_result;
    }

    if (!semaphore_.try_acquire()) {
      l_result.status_ = doodle::seedance2::task_status::preparing;
      co_return l_result;
    }
    {
      hash_map_t::accessor l_accessor;
      map_.insert(l_accessor, in_task.uuid_id_);
      l_accessor->second.status_ = doodle::seedance2::task_status::running;
    }

    boost::asio::co_spawn(
        g_io_context(),
        [in_client = std::move(in_client), in_task]() -> boost::asio::awaitable<void> {
          boost::scope::scope_exit l_release{[]() { get().semaphore_.release(); }};
          ai_client_base::run_task_result_t l_result{};
          l_result.status_ = doodle::seedance2::task_status::failed;
          try {
            l_result = co_await in_client->submit_task(in_task);
          } catch (...) {
            l_result.data_response_ = nlohmann::json{{"error", boost::current_exception_diagnostic_information()}};
          }
          hash_map_t::accessor l_accessor;
          if (get().map_.find(l_accessor, in_task.uuid_id_)) l_accessor->second = std::move(l_result);
          co_return;
        },
        boost::asio::detached
    );

    l_result.status_ = doodle::seedance2::task_status::running;
    co_return l_result;
  }

  std::optional<ai_client_base::run_task_result_t> take(const uuid& in_id) {
    hash_map_t::accessor l_accessor;
    if (!map_.find(l_accessor, in_id)) return std::nullopt;
    const auto l_status = l_accessor->second.status_;
    if (l_status == doodle::seedance2::task_status::running || l_status == doodle::seedance2::task_status::queued ||
        l_status == doodle::seedance2::task_status::preparing)
      return std::nullopt;
    auto l_result = std::move(l_accessor->second);
    map_.erase(l_accessor);
    return l_result;
  }

 private:
  using hash_map_t  = tbb::concurrent_hash_map<uuid, ai_client_base::run_task_result_t>;
  using semaphore_t = std::counting_semaphore<g_max_inflight>;

  hash_map_t map_{};
  semaphore_t semaphore_{g_max_inflight};
};

struct compressed_image_t {
  std::string b64_{};
  std::int32_t cols_{};
  std::int32_t rows_{};
};

compressed_image_t compress_reference_image(const FSys::path& in_path) {
  cv::Mat l_image = cv::imread(in_path.generic_string(), cv::IMREAD_UNCHANGED);
  DOODLE_CHICK(!l_image.empty(), "参考图解码失败: {}", in_path.generic_string());

  std::vector<uchar> l_buffer{};
  DOODLE_CHICK(
      cv::imencode(std::string{g_compressed_ext}, l_image, l_buffer, {cv::IMWRITE_PNG_COMPRESSION, 9}) &&
          !l_buffer.empty(),
      "参考图编码失败: {}", in_path.generic_string()
  );

  // interactions 的 image block 要的是 base64 文本, 不是原始字节 —— 直接把 PNG 字节塞进 JSON 会被
  // nlohmann 按 UTF-8 序列化, 非法序列会被替换, 上游拿到的是损坏数据。
  auto l_b64 = base64_encode(std::string_view{reinterpret_cast<const char*>(l_buffer.data()), l_buffer.size()});
  DOODLE_CHICK(!l_b64.empty(), "参考图 base64 编码失败: {}", in_path.generic_string());

  return compressed_image_t{.b64_ = std::move(l_b64), .cols_ = l_image.cols, .rows_ = l_image.rows};
}

std::string to_lower_copy(std::string_view in_str) {
  std::string l_str{in_str};
  std::ranges::transform(l_str, l_str.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return l_str;
}

void collect_images_from_content(const nlohmann::json& in_content, std::vector<gemini_image_client::output_image_t>& out) {
  if (!in_content.is_array()) return;
  for (const auto& l_block : in_content) {
    if (!l_block.is_object()) continue;
    if (!l_block.contains("type") || !l_block.at("type").is_string()) continue;
    if (l_block.at("type").get<std::string>() != g_content_image) continue;
    if (!l_block.contains("data") || !l_block.at("data").is_string()) continue;

    gemini_image_client::output_image_t l_image{};
    l_image.data_      = l_block.at("data").get<std::string>();
    l_image.mime_type_ = l_block.contains("mime_type") && l_block.at("mime_type").is_string()
                             ? l_block.at("mime_type").get<std::string>()
                             : std::string{g_response_mime};
    out.push_back(std::move(l_image));
  }
}

void collect_text_from_content(const nlohmann::json& in_content, std::string& out) {
  if (!in_content.is_array()) return;
  for (const auto& l_block : in_content) {
    if (!l_block.is_object()) continue;
    if (!l_block.contains("type") || !l_block.at("type").is_string()) continue;
    if (l_block.at("type").get<std::string>() != g_content_text) continue;
    if (!l_block.contains("text") || !l_block.at("text").is_string()) continue;
    if (!out.empty()) out += "\n";
    out += l_block.at("text").get<std::string>();
  }
}

}  // namespace

gemini_image_client::request_t::request_t(const doodle::seedance2::transfer_station_request_t& in_req)
    : model_(in_req.model_.empty() ? std::string{g_default_model} : in_req.model_),
      prompt_(in_req.prompt_),
      // 宽高比与分辨率都只作用在 response_format 上: 提示词里再写一遍 "aspect ratio 16:9" 就重复了,
      // 容易被模型当成两句不同要求。
      aspect_ratio_(normalize_aspect_ratio(in_req.aspect_ratio_)),
      image_size_(normalize_image_size(in_req.image_size_)),
      thinking_level_(normalize_thinking_level(in_req.quality_)) {}

nlohmann::json gemini_image_client::request_t::to_body() const {
  return nlohmann::json{} = *this;
}

std::string gemini_image_client::normalize_aspect_ratio(std::string_view in_ratio) {
  auto l_ratio = to_lower_copy(in_ratio);
  if (std::ranges::find(g_aspect_ratios, std::string_view{l_ratio}) == g_aspect_ratios.end())
    throw_exception(
        http_request_error{
            boost::beast::http::status::bad_request,
            fmt::format("Gemini 不支持的宽高比 {}, 可选: {}", in_ratio, fmt::join(g_aspect_ratios, " / "))
        }
    );
  return l_ratio;
}

std::string gemini_image_client::normalize_image_size(std::string_view in_size) {
  if (in_size.empty()) return {};
  if (std::ranges::find(g_image_sizes, in_size) != g_image_sizes.end()) return std::string{in_size};

  throw_exception(
      http_request_error{
          boost::beast::http::status::bad_request,
          fmt::format("Gemini 不支持的分辨率 {}, 可选: {}", in_size, fmt::join(g_image_sizes, " / "))
      }
  );
}

std::string gemini_image_client::normalize_thinking_level(std::string_view in_quality) {
  // quality 是公共请求里的通用字段(API易 那边原样透传), Gemini 这边把它当「思考深度」用。
  auto l_quality = to_lower_copy(in_quality);
  std::erase(l_quality, ' ');
  // 留空表示不下发, 沿用模型默认级别 —— 显式写死 medium 会把上游以后改默认值的余地钉死。
  if (l_quality.empty()) return {};

  // 已是合法级别就原样返回
  if (std::ranges::find(g_thinking_levels, std::string_view{l_quality}) != g_thinking_levels.end()) return l_quality;

  if (l_quality == "low" || l_quality == "fast" || l_quality == "draft") return std::string{"minimal"};
  if (l_quality == "medium" || l_quality == "standard" || l_quality == "normal") return std::string{"medium"};
  if (l_quality == "high" || l_quality == "hd" || l_quality == "vip" || l_quality == "best")
    return std::string{"high"};

  throw_exception(
      http_request_error{
          boost::beast::http::status::bad_request,
          fmt::format("Gemini 不支持的 quality {}, 可选: {}", in_quality, fmt::join(g_thinking_levels, " / "))
      }
  );
}

std::string gemini_image_client::mime_of_image(const std::string& in_ext) {
  auto l_ext = to_lower_copy(in_ext);
  if (l_ext == ".jpg" || l_ext == ".jpeg") return "image/jpeg";
  if (l_ext == ".webp") return "image/webp";
  if (l_ext == ".heic") return "image/heic";
  if (l_ext == ".heif") return "image/heif";
  return std::string{g_default_mime};
}

std::string gemini_image_client::ext_of_mime(std::string_view in_mime) {
  auto l_mime = to_lower_copy(in_mime);
  if (l_mime == "image/jpeg" || l_mime == "image/jpg") return ".jpg";
  if (l_mime == "image/webp") return ".webp";
  if (l_mime == "image/heic") return ".heic";
  if (l_mime == "image/heif") return ".heif";
  return std::string{g_response_ext};
}

void to_json(nlohmann::json& j, const gemini_image_client::request_t& req) {
  // interactions 的 input 是一个 content 数组: 提示词在前, 参考图在后, 与官方示例同序。
  nlohmann::json l_content = nlohmann::json::array();
  l_content.push_back(nlohmann::json{{"type", std::string{g_content_text}}, {"text", req.prompt_}});
  for (const auto& l_ref : req.references_) {
    l_content.push_back(
        nlohmann::json{
            {"type", std::string{g_content_image}},
            {"data", l_ref.b64_},
            {"mime_type", l_ref.mime_},
        }
    );
  }

  nlohmann::json l_response_format{
      {"type", std::string{g_image_response_type}},
      {"mime_type", std::string{g_response_mime}},
  };
  if (!req.aspect_ratio_.empty()) l_response_format["aspect_ratio"] = req.aspect_ratio_;
  if (!req.image_size_.empty()) l_response_format["image_size"] = req.image_size_;

  j = nlohmann::json{
      {"model", req.model_},
      {"input", std::move(l_content)},
      {"response_format", std::move(l_response_format)},
  };
  // quality 留空时不下发 generation_config, 请求体与不传 quality 完全一致。
  if (!req.thinking_level_.empty())
    j["generation_config"] = nlohmann::json{{"thinking_level", req.thinking_level_}};
}

FSys::path gemini_image_client::parse_site_image_id(std::string_view in_url) {
  if (!in_url.starts_with(g_pictures_prefix)) return {};

  FSys::path l_name{in_url};
  auto l_ext    = to_lower_copy(l_name.extension().generic_string());
  auto l_id_str = l_name.stem().generic_string();

  auto l_id     = from_uuid_str(l_id_str);

  return g_ctx().get<kitsu_ctx_t>().get_sd2_pictures_file(l_id, l_ext);
}

nlohmann::json gemini_image_client::strip_image_payload(nlohmann::json in_body) {
  if (!in_body.is_object()) return in_body;

  for (const auto* l_key : {"steps", "output"}) {
    if (!in_body.contains(l_key)) continue;
    auto& l_steps = in_body.at(l_key);
    if (!l_steps.is_array()) continue;
    for (auto& l_step : l_steps) {
      if (!l_step.is_object()) continue;
      for (const auto* l_content_key : {"content", "summary"}) {
        if (!l_step.contains(l_content_key)) continue;
        auto& l_content = l_step.at(l_content_key);
        if (!l_content.is_array()) continue;
        for (auto& l_block : l_content)
          if (l_block.is_object() && l_block.contains("data")) l_block["data"] = g_payload_placeholder;
      }
    }
  }

  // 输入回显里也可能带着几十 MB 的参考图
  if (in_body.contains("input")) {
    auto& l_input = in_body.at("input");
    if (l_input.is_array())
      for (auto& l_block : l_input)
        if (l_block.is_object() && l_block.contains("data")) l_block["data"] = g_payload_placeholder;
  }
  return in_body;
}

gemini_image_client::response_t gemini_image_client::parse_response(const nlohmann::json& in_body) {
  response_t l_res{};
  if (!in_body.is_object()) return l_res;

  // REST 的错误体有两种: Google 标准封套 {"error": {"code","message","status"}},
  // 以及个别情况下直接是 error 字符串。两种都要认, 否则前端只能看到一个空的失败。
  if (in_body.contains("error")) {
    l_res.has_error_  = true;
    const auto& l_err = in_body.at("error");
    if (l_err.is_string()) {
      l_res.error_message_ = l_err.get<std::string>();
      l_res.error_code_    = l_res.error_message_;
    } else if (l_err.is_object()) {
      if (l_err.contains("status") && l_err.at("status").is_string())
        l_res.error_code_ = l_err.at("status").get<std::string>();
      if (l_res.error_code_.empty() && l_err.contains("code") && l_err.at("code").is_number_integer())
        l_res.error_code_ = std::to_string(l_err.at("code").get<std::int64_t>());
      if (l_err.contains("message") && l_err.at("message").is_string())
        l_res.error_message_ = l_err.at("message").get<std::string>();
      if (l_res.error_code_.empty()) l_res.error_code_ = l_res.error_message_;
    }
  }

  if (in_body.contains("id") && in_body.at("id").is_string())
    l_res.interaction_id_ = in_body.at("id").get<std::string>();
  if (in_body.contains("status") && in_body.at("status").is_string())
    l_res.status_ = in_body.at("status").get<std::string>();

  if (in_body.contains("steps") && in_body.at("steps").is_array()) {
    for (const auto& l_step : in_body.at("steps")) {
      if (!l_step.is_object()) continue;
      // 只要 model_output: thought 步里的图是中间稿, 不是最终结果。
      if (!l_step.contains("type") || !l_step.at("type").is_string()) continue;
      if (l_step.at("type").get<std::string>() != g_step_model_output) continue;
      if (!l_step.contains("content")) continue;
      collect_text_from_content(l_step.at("content"), l_res.text_);
      collect_images_from_content(l_step.at("content"), l_res.images_);
    }
  }

  if (in_body.contains("usage") && in_body.at("usage").is_object()) {
    const auto& l_usage  = in_body.at("usage");
    l_res.input_tokens_  = l_usage.value("total_input_tokens", std::int64_t{0});
    l_res.output_tokens_ = l_usage.value("total_output_tokens", std::int64_t{0});
    l_res.total_tokens_  = l_usage.value("total_tokens", std::int64_t{0});
    if (l_res.total_tokens_ == 0) l_res.total_tokens_ = l_res.input_tokens_ + l_res.output_tokens_;
  }
  return l_res;
}

doodle::seedance2::task_status gemini_image_client::response_t::get_status(int in_status_code) const {
  const bool l_http_ok = in_status_code >= 200 && in_status_code < 300;
  if (!l_http_ok) return doodle::seedance2::task_status::failed;
  if (has_error_) return doodle::seedance2::task_status::failed;
  // status 是 completed 但没有图, 就是被安全策略拦下了: 这种情况**是终态失败**, 不能当 running
  // 让调用方一轮轮重试 —— 同一段提示词每轮都会被拦, 只会白烧配额。
  if (has_images()) return doodle::seedance2::task_status::succeeded;
  return doodle::seedance2::task_status::failed;
}

FSys::path gemini_image_client::write_b64_to_cache(std::string_view in_b64, std::string_view in_ext) const {
  if (in_b64.starts_with("data:"))
    if (const auto l_comma = in_b64.find(','); l_comma != std::string_view::npos) in_b64.remove_prefix(l_comma + 1);

  auto l_path  = core_set::get_set().get_cache_root("http") /
                 (core_set::get_set().get_uuid_str() + std::string{in_ext.empty() ? g_default_ext : in_ext});

  auto l_bytes = base64_decode(in_b64);
  DOODLE_CHICK(!l_bytes.empty(), "图片 base64 解码结果为空(长度 {})", in_b64.size());

  std::ofstream l_ofs{l_path, std::ios::binary | std::ios::trunc};
  DOODLE_CHICK(l_ofs.is_open(), "无法写入图片缓存文件: {}", l_path.generic_string());
  l_ofs.write(l_bytes.data(), static_cast<std::streamsize>(l_bytes.size()));
  DOODLE_CHICK(l_ofs.good(), "写入图片缓存失败: {}", l_path.generic_string());
  l_ofs.close();

  SPDLOG_LOGGER_INFO(logger_, "Gemini 图片已落盘: {} ({} 字节)", l_path.generic_string(), l_bytes.size());
  return l_path;
}

gemini_image_client::reference_image_t gemini_image_client::load_reference_image(std::string_view in_url) {
  reference_image_t l_ref{};
  l_ref.url_              = std::string{in_url};
  const auto l_local_path = parse_site_image_id(in_url);

  const auto l_compressed = compress_reference_image(l_local_path);
  l_ref.b64_              = std::move(l_compressed.b64_);
  l_ref.mime_             = std::string{g_default_mime};

  SPDLOG_LOGGER_INFO(
      logger_, "Gemini 参考图已压缩: {} ({}x{}), base64 {} 字节", l_ref.url_, l_compressed.cols_,
      l_compressed.rows_, l_ref.b64_.size()
  );
  return l_ref;
}

ai_client_base::request_info_t gemini_image_client::collect_request_info(const nlohmann::json& in_request) const {
  const auto l_req = in_request.get<doodle::seedance2::transfer_station_request_t>();

  for (const auto& url : l_req.reference_images_) {
    const auto l_path = parse_site_image_id(url);
    // 扩展名先查: 路由对不上与扩展名不支持都要报 400, 但后端两者都返回空路径,
    // 先判扩展名才能让调用方知道到底哪条规则没过。
    auto l_ext = l_path.extension();
    if (l_ext != ".png" && l_ext != ".jpg" && l_ext != ".jpeg")
      throw_exception(
          http_request_error{
              boost::beast::http::status::bad_request,
              fmt::format("参考图扩展名必须是 .png/.jpg/.jpeg 之一, 收到: {}", std::string{url})
          }
      );
    if (l_path.empty() || !FSys::exists(l_path))
      throw_exception(
          http_request_error{
              boost::beast::http::status::bad_request,
              fmt::format(
                  "参考图 URL 必须指向本站图片路由 {}<uuid><扩展名> 且文件存在, 收到: {}", g_pictures_prefix,
                  std::string{url}
              )
          }
      );
    const auto l_size = FSys::file_size(l_path);
    DOODLE_CHICK_HTTP(
        l_size > 0, bad_request,
        fmt::format("参考图大小必须大于 0 字节, 收到 {} 字节: {}", l_size, std::string{url})
    );
  }

  const request_t l_gemini_req{l_req};

  request_info_t l_info{};
  l_info.model_       = l_gemini_req.model_;
  l_info.resolution_  = l_gemini_req.image_size_.empty() ? l_gemini_req.aspect_ratio_ : l_gemini_req.image_size_;
  l_info.text_prompt_ = l_gemini_req.prompt_;
  return l_info;
}

std::size_t gemini_image_client::default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const {
  switch (in_task_type) {
    case doodle::seedance2::task_type::video:
      return doodle_config::g_max_task_completion_tokens;
    case doodle::seedance2::task_type::inference:
      return doodle_config::g_ark_inference_default_consumed_tokens;
    default:
      return doodle_config::g_ark_inference_default_consumed_tokens;
  }
}

std::shared_ptr<doodle::http::http_client_ssl> gemini_image_client::make_own_client() const {
  auto l_ctx = core_set::get_set().ctx_ptr;
  DOODLE_CHICK(l_ctx != nullptr, "ssl context 未初始化(见 core_set::ctx_ptr), 无法创建 Gemini 客户端");
  return std::make_shared<doodle::http::http_client_ssl>(std::string{g_default_host}, *l_ctx);
}

boost::asio::awaitable<ai_client_base::run_task_result_t> gemini_image_client::run_task(
    const doodle::seedance2::task& in_task
) {
  // 已经跑过一轮(状态 running)就只取结果, 不重复发请求 —— 与 API易 后端的队列语义一致。
  return gemini_queue_t::get().acquire(shared_from_this(), in_task);
}

boost::asio::awaitable<ai_client_base::run_task_result_t> gemini_image_client::submit_task(
    const doodle::seedance2::task& in_task
) {
  run_task_result_t l_result{};
  l_result.status_ = doodle::seedance2::task_status::failed;

  if (in_task.type_ == doodle::seedance2::task_type::inference) {
    logger_->error("Gemini 图片后端不支持推理任务");
    l_result.data_response_ = nlohmann::json{{"error", "Gemini 图片后端不支持推理任务"}};
    co_return l_result;
  }
  if (in_task.type_ == doodle::seedance2::task_type::video) {
    logger_->error("Gemini 图片后端不支持视频任务");
    l_result.data_response_ = nlohmann::json{{"error", "Gemini 图片后端不支持视频任务"}};
    co_return l_result;
  }

  const auto l_req_body = in_task.data_request_.get<doodle::seedance2::transfer_station_request_t>();
  request_t l_gemini_req{l_req_body};
  for (const auto& l_url : l_req_body.reference_images_)
    l_gemini_req.references_.push_back(load_reference_image(l_url));

  boost::beast::http::request<boost::beast::http::string_body> l_req{
      boost::beast::http::verb::post, std::string{g_interactions_path}, 11
  };
  l_req.body() = l_gemini_req.to_body().dump();
  l_req.set(boost::beast::http::field::content_type, "application/json");

  if (!l_gemini_req.references_.empty())
    SPDLOG_LOGGER_INFO(
        logger_, "Gemini 图片编辑: {} 张参考图, 请求体 {} 字节", l_gemini_req.references_.size(), l_req.body().size()
    );

  auto l_client = make_own_client();

  // Gemini 用 x-goog-api-key 头鉴权, 不是 Authorization: Bearer —— 两家的凭据不能互相顶替。
  l_req.set("x-goog-api-key", token_);
  l_req.set(boost::beast::http::field::accept, "application/json");
  l_req.set(boost::beast::http::field::host, l_client->server_ip_);
  l_req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
  // 出图(base64 内联)常有几十 MB, 放宽 body 上限, 否则 beast 会以 body limit 超限报错。
  l_client->body_limit_ = 256ull * 1024 * 1024;
  l_client->set_timeout(g_request_timeout);

  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await l_client->read_and_write(l_req, l_res, boost::asio::use_awaitable);

  l_result.data_response_     = strip_image_payload(l_res.body());
  const auto l_response       = parse_response(l_res.body());
  l_result.status_            = l_response.get_status(static_cast<int>(l_res.result()));
  l_result.task_id_           = l_response.interaction_id_;
  l_result.error_code_        = l_response.error_code_;
  l_result.input_tokens_      = l_response.input_tokens_;
  l_result.output_tokens_     = l_response.output_tokens_;
  l_result.total_tokens_      = l_response.total_tokens_;
  l_result.completion_tokens_ = l_response.total_tokens_;
  l_result.result_text_       = l_response.text_;

  if (l_result.status_ != doodle::seedance2::task_status::succeeded) {
    // 被安全策略拦下时 HTTP 是 200 而 body 里只有一段文字: 把那段文字带上, 否则前端只知道"失败"。
    if (!l_response.text_.empty()) l_result.data_response_["text"] = l_response.text_;
    SPDLOG_LOGGER_ERROR(
        logger_, "Gemini 图片生成失败: HTTP {} status `{}` error_code `{}` body {}", l_res.result(),
        l_response.status_, l_result.error_code_, l_result.data_response_.dump()
    );
    co_return l_result;
  }

  DOODLE_CHICK(l_response.has_images(), "Gemini 图片响应里没有任何 image 内容块");
  for (const auto& l_image : l_response.images_)
    l_result.result_file_paths_.push_back(write_b64_to_cache(l_image.data_, ext_of_mime(l_image.mime_type_)));

  SPDLOG_LOGGER_INFO(
      logger_, "Gemini 图片生成完成: {} 个文件, tokens {}/{}", l_result.result_file_paths_.size(),
      l_result.input_tokens_, l_result.output_tokens_
  );
  co_return l_result;
}

boost::asio::awaitable<ai_client_base::run_task_result_t> gemini_image_client::query_task(
    const doodle::seedance2::task& in_task
) {
  throw_exception(doodle_error{"gemini_image_client 是同步后端, 结果只在 run_task 的返回值里, 没有 query_task"});
  co_return run_task_result_t{};
}

boost::asio::awaitable<void> gemini_image_client::cancel_task(const std::string& in_task_id) {
  throw_exception(doodle_error{"gemini_image_client 是同步后端, 没有任务可取消, 不支持 cancel_task"});
  co_return;
}

boost::asio::awaitable<void> gemini_image_client::download_result(run_task_result_t& in_data) {
  throw_exception(doodle_error{"gemini_image_client 的结果文件在 run_task 里已落盘, 不支持 download_result"});
  co_return;
}

}  // namespace doodle::http::seedance2
