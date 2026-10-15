#include "apipy_image_client.h"

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
#include <array>
#include <cctype>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <iterator>
#include <optional>
#include <ranges>
#include <semaphore>
#include <string>
#include <tbb/concurrent_hash_map.h>
#include <utility>

namespace doodle::http::seedance2 {

namespace {

constexpr std::string_view g_generations_path{"/v1/images/generations"};
constexpr std::string_view g_edits_path{"/v1/images/edits"};
constexpr std::string_view g_default_ext{".png"};
constexpr std::string_view g_pictures_prefix{"/api/seedance2/pictures/"};
constexpr std::chrono::seconds g_request_timeout{360};
constexpr std::string_view g_default_model{"gpt-image-2.5-vip"};
constexpr std::string_view g_edit_image_field{"image"};
constexpr std::size_t g_max_reference_images{8};
constexpr std::uintmax_t g_max_reference_image_bytes{1024 * 1024};

const nlohmann::json& first_data_item(const nlohmann::json& in_body) {
  static const nlohmann::json l_empty = nlohmann::json::object();
  if (!in_body.is_object()) return l_empty;
  if (!in_body.contains("data")) return l_empty;
  const auto& l_data = in_body.at("data");
  if (!l_data.is_array() || l_data.empty()) return l_empty;
  const auto& l_item = l_data.front();
  return l_item.is_object() ? l_item : l_empty;
}

class apipy_queue_t {
 public:
  static constexpr std::ptrdiff_t g_max_inflight{10};

  static apipy_queue_t& get() {
    static apipy_queue_t l_instance{};
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

}  // namespace

std::string apipy_image_client::mime_of_image(const std::string& in_ext) {
  auto l_ext = in_ext;
  std::ranges::transform(l_ext, l_ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (l_ext == ".jpg" || l_ext == ".jpeg") return "image/jpeg";
  if (l_ext == ".webp") return "image/webp";
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

void to_json(nlohmann::json& j, const apipy_image_client::request_t& req) {
  j["model"]  = req.model_;
  j["prompt"] = req.prompt_;
  j["size"]   = req.size_;
  if (!req.quality_.empty()) j["quality"] = req.quality_;
}

apipy_image_client::edits_form_t apipy_image_client::request_t::to_multipart() const {
  edits_form_t l_form{};
  l_form.boundary_ = "----doodle" + boost::uuids::to_string(core_set::get_set().get_uuid());

  const std::array<std::pair<std::string_view, std::string_view>, 4> l_text_fields{{
      {"model", model_},
      {"prompt", prompt_},
      {"size", size_},
      {"response_format", "url"},
  }};

  for (const auto& [l_key, l_value] : l_text_fields) {
    l_form.body_ += "--";
    l_form.body_ += l_form.boundary_;
    l_form.body_ += "\r\nContent-Disposition: form-data; name=\"";
    l_form.body_ += l_key;
    l_form.body_ += "\"\r\n\r\n";
    l_form.body_ += l_value;
    l_form.body_ += "\r\n";
  }

  if (!quality_.empty()) {
    l_form.body_ += "--";
    l_form.body_ += l_form.boundary_;
    l_form.body_ += "\r\nContent-Disposition: form-data; name=\"quality\"\r\n\r\n";
    l_form.body_ += quality_;
    l_form.body_ += "\r\n";
  }

  for (const auto& l_ref : image_bytes_) {
    std::string l_filename{};
    l_filename.reserve(l_ref.filename_.size());
    for (const auto l_c : l_ref.filename_)
      if (l_c != '\r' && l_c != '\n') l_filename.push_back(l_c);

    l_form.body_ += "--";
    l_form.body_ += l_form.boundary_;
    l_form.body_ += "\r\nContent-Disposition: form-data; name=\"";
    l_form.body_ += g_edit_image_field;
    l_form.body_ += "\"; filename=\"";
    l_form.body_ += l_filename;
    l_form.body_ += "\"\r\nContent-Type: ";
    l_form.body_ += l_ref.mime_;
    l_form.body_ += "\r\n\r\n";
    l_form.body_ += l_ref.bytes_;
    l_form.body_ += "\r\n";
  }

  l_form.body_ += "--";
  l_form.body_ += l_form.boundary_;
  l_form.body_ += "--\r\n";
  return l_form;
}

apipy_image_client::response_t apipy_image_client::parse_response(const nlohmann::json& in_body) {
  response_t l_res{};
  if (!in_body.is_object()) return l_res;

  if (in_body.contains("error")) {
    l_res.has_error_  = true;
    const auto& l_err = in_body.at("error");
    if (l_err.is_string()) {
      l_res.error_message_ = l_err.get<std::string>();
    } else if (l_err.is_object()) {
      for (const auto* l_key : {"code", "type"}) {
        if (l_err.contains(l_key) && l_err.at(l_key).is_string()) {
          l_res.error_code_ = l_err.at(l_key).get<std::string>();
          break;
        }
      }
      if (l_err.contains("message") && l_err.at("message").is_string())
        l_res.error_message_ = l_err.at("message").get<std::string>();
      if (l_res.error_code_.empty()) l_res.error_code_ = l_res.error_message_;
    }
  }

  const auto& l_item = first_data_item(in_body);
  if (l_item.contains("url") && l_item.at("url").is_string()) l_res.url_ = l_item.at("url").get<std::string>();
  if (l_item.contains("b64_json") && l_item.at("b64_json").is_string())
    l_res.b64_json_ = l_item.at("b64_json").get<std::string>();
  if (l_res.b64_json_.starts_with("data:"))
    if (const auto l_comma = l_res.b64_json_.find(','); l_comma != std::string::npos)
      l_res.b64_json_.erase(0, l_comma + 1);

  if (in_body.contains("usage") && in_body.at("usage").is_object()) {
    const auto& l_usage  = in_body.at("usage");
    l_res.input_tokens_  = l_usage.value("input_tokens", std::int64_t{0});
    l_res.output_tokens_ = l_usage.value("output_tokens", std::int64_t{0});
    l_res.total_tokens_  = l_usage.value("total_tokens", std::int64_t{0});
  }
  return l_res;
}

doodle::seedance2::task_status apipy_image_client::response_t::get_status(int in_status_code) const {
  const bool l_http_ok = in_status_code >= 200 && in_status_code < 300;
  if (!l_http_ok) return doodle::seedance2::task_status::failed;
  if (has_error_) return doodle::seedance2::task_status::failed;
  if (has_url() || has_b64_json()) return doodle::seedance2::task_status::succeeded;
  return doodle::seedance2::task_status::failed;
}

FSys::path apipy_image_client::write_b64_to_cache(std::string_view in_b64, std::string_view in_ext) const {
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

  l_ref.mime_     = mime_of_image(l_local_path.extension().generic_string());
  return l_ref;
}

ai_client_base::request_info_t apipy_image_client::collect_request_info(const nlohmann::json& in_request) const {
  const auto l_req = in_request.get<doodle::seedance2::transfer_station_request_t>();

  DOODLE_CHICK_HTTP(
      l_req.reference_images_.size() <= g_max_reference_images, bad_request,
      fmt::format("参考图最多 {} 张, 收到 {} 张", g_max_reference_images, l_req.reference_images_.size())
  );

  for (const auto& url : l_req.reference_images_) {
    const auto l_path = parse_site_image_id(url);
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
        l_size > 0 && l_size <= g_max_reference_image_bytes, bad_request,
        fmt::format(
            "参考图大小必须在 1 字节到 {} 字节(1M) 之间, 收到 {} 字节: {}", g_max_reference_image_bytes, l_size,
            std::string{url}
        )
    );
    auto l_ext = l_path.extension().generic_string();
    std::ranges::transform(l_ext, l_ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (l_ext != ".png" && l_ext != ".jpg" && l_ext != ".jpeg" && l_ext != ".webp")
      throw_exception(
          http_request_error{
              boost::beast::http::status::bad_request,
              fmt::format("参考图扩展名必须是 .png/.jpg/.jpeg/.webp 之一, 收到: {}", std::string{url})
          }
      );
  }

  request_info_t l_info{};
  l_info.model_       = l_req.model_;
  l_info.resolution_  = l_req.aspect_ratio_.empty() ? l_req.image_size_ : l_req.aspect_ratio_;
  l_info.text_prompt_ = l_req.prompt_;

  DOODLE_CHICK_HTTP(!l_info.model_.empty(), bad_request, "缺少模型名称");
  DOODLE_CHICK_HTTP(!l_info.resolution_.empty(), bad_request, "缺少分辨率");
  DOODLE_CHICK_HTTP(!l_info.text_prompt_.empty(), bad_request, "缺少提示词");

  return l_info;
}

std::size_t apipy_image_client::default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const {
  switch (in_task_type) {
    case doodle::seedance2::task_type::video:
      return doodle_config::g_max_task_completion_tokens;
    case doodle::seedance2::task_type::inference:
      return doodle_config::g_ark_inference_default_consumed_tokens;
    default:
      return doodle_config::g_ark_inference_default_consumed_tokens;
  }
}

std::shared_ptr<doodle::http::http_client_ssl> apipy_image_client::make_own_client() const {
  auto l_ctx = core_set::get_set().ctx_ptr;
  DOODLE_CHICK(l_ctx != nullptr, "ssl context 未初始化(见 core_set::ctx_ptr), 无法创建 API易 客户端");
  return std::make_shared<doodle::http::http_client_ssl>(std::string{g_default_host}, *l_ctx);
}

boost::asio::awaitable<ai_client_base::run_task_result_t> apipy_image_client::run_task(
    const doodle::seedance2::task& in_task
) {
  auto& l_queue = apipy_queue_t::get();
  return l_queue.acquire(shared_from_this(), in_task);
}

boost::asio::awaitable<ai_client_base::run_task_result_t> apipy_image_client::submit_task(
    const doodle::seedance2::task& in_task
) {
  run_task_result_t l_result{};
  l_result.status_ = doodle::seedance2::task_status::failed;

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

  const auto l_req_body    = in_task.data_request_.get<doodle::seedance2::transfer_station_request_t>();
  const auto& l_references = l_req_body.reference_images_;
  apipy_image_client::request_t l_apipy_req{l_req_body};

  boost::beast::http::request<boost::beast::http::string_body> l_req{boost::beast::http::verb::post, {}, 11};
  if (l_references.empty()) {
    auto l_body = (nlohmann::json{} = l_apipy_req);
    l_req.target(std::string{g_generations_path});
    l_req.body() = l_body.dump();
    l_req.set(boost::beast::http::field::content_type, "application/json");
  } else {
    for (const auto& l_url : l_references) l_apipy_req.image_bytes_.push_back(load_reference_image(l_url));

    auto l_form = l_apipy_req.to_multipart();
    l_req.target(std::string{g_edits_path});
    l_req.body() = std::move(l_form.body_);
    l_req.set(boost::beast::http::field::content_type, l_form.content_type());

    SPDLOG_LOGGER_INFO(
        logger_, "API易 图片编辑: {} 张参考图, 请求体 {} 字节", l_apipy_req.image_bytes_.size(), l_req.body().size()
    );
  }

  auto l_client = make_own_client();

  l_req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token_));
  l_req.set(boost::beast::http::field::accept, "application/json");
  l_req.set(boost::beast::http::field::host, l_client->server_ip_);
  l_req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
  l_client->set_timeout(g_request_timeout);

  boost::beast::http::response<http::basic_json_body> l_res{};
  try {
    co_await l_client->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  } catch (...) {
    throw_exception(
        http_request_error{
            boost::beast::http::status::internal_server_error, boost::current_exception_diagnostic_information()
        }
    );
  }

  l_result.data_response_ = l_res.body();
  const auto l_response   = parse_response(l_result.data_response_);
  l_result.status_        = l_response.get_status(static_cast<int>(l_res.result()));
  l_result.error_code_    = l_response.error_code_;
  l_result.input_tokens_  = l_response.input_tokens_;
  l_result.output_tokens_ = l_response.output_tokens_;
  l_result.total_tokens_  = l_response.total_tokens_;

  if (l_result.status_ != doodle::seedance2::task_status::succeeded) {
    SPDLOG_LOGGER_ERROR(
        logger_, "API易 图片生成失败: HTTP {} error_code `{}` body {}", l_res.result(), l_result.error_code_,
        l_result.data_response_.dump()
    );
    co_return l_result;
  }

  if (l_response.has_url()) {
    l_result.result_files_.push_back(l_response.url_);
    l_result.result_file_paths_.push_back(co_await download_raw_file(l_response.url_));
  } else if (l_response.has_b64_json()) {
    l_result.result_file_paths_.push_back(write_b64_to_cache(l_response.b64_json_, g_default_ext));
  } else {
    DOODLE_CHICK(false, "API易 图片响应里既没有 url 也没有 b64_json");
  }

  SPDLOG_LOGGER_INFO(
      logger_, "API易 图片生成完成: {} 个文件, tokens {}/{}", l_result.result_file_paths_.size(),
      l_result.input_tokens_, l_result.output_tokens_
  );
  co_return l_result;
}

boost::asio::awaitable<ai_client_base::run_task_result_t> apipy_image_client::query_task(
    const doodle::seedance2::task& in_task
) {
  throw_exception(doodle_error{"apipy_image_client 是同步后端, 结果只在 run_task 的返回值里, 没有 query_task"});
  co_return run_task_result_t{};
}

boost::asio::awaitable<void> apipy_image_client::cancel_task(const std::string& in_task_id) {
  throw_exception(doodle_error{"apipy_image_client 是同步后端, 没有任务可取消, 不支持 cancel_task"});
  co_return;
}

boost::asio::awaitable<void> apipy_image_client::download_result(run_task_result_t& in_data) {
  throw_exception(doodle_error{"apipy_image_client 的结果文件在 run_task 里已落盘, 不支持 download_result"});
  co_return;
}

}  // namespace doodle::http::seedance2
