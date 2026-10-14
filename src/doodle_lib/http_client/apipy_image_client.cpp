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

constexpr std::string_view g_generations_path{"/v1/images/generations"};
constexpr std::string_view g_edits_path{apipy_image_client::g_edits_path};

constexpr std::string_view g_default_ext{".png"};

constexpr std::string_view g_pictures_prefix{"/api/seedance2/pictures/"};

std::string make_boundary() { return "----doodle" + boost::uuids::to_string(core_set::get_set().get_uuid()); }

std::string strip_crlf(std::string_view in_text) {
  std::string l_out{};
  l_out.reserve(in_text.size());
  for (const auto l_c : in_text)
    if (l_c != '\r' && l_c != '\n') l_out.push_back(l_c);
  return l_out;
}

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

const nlohmann::json& first_data_item(const nlohmann::json& in_body) {
  static const nlohmann::json l_empty = nlohmann::json::object();
  if (!in_body.is_object()) return l_empty;
  if (!in_body.contains("data")) return l_empty;
  const auto& l_data = in_body.at("data");
  if (!l_data.is_array() || l_data.empty()) return l_empty;
  const auto& l_item = l_data.front();
  return l_item.is_object() ? l_item : l_empty;
}

std::optional<std::string> image_url_of_content_item(const nlohmann::json& in_item) {
  if (!in_item.is_object() || !in_item.contains("image_url")) return std::nullopt;
  const auto& l_holder = in_item.at("image_url");
  if (l_holder.is_object() && l_holder.contains("url") && l_holder.at("url").is_string())
    return l_holder.at("url").get<std::string>();
  if (l_holder.is_string()) return l_holder.get<std::string>();
  return std::nullopt;
}

}  // namespace

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
  if (l_b64.starts_with("data:"))
    if (const auto l_comma = l_b64.find(','); l_comma != std::string::npos) l_b64.erase(0, l_comma + 1);
  return l_b64;
}

std::string apipy_image_client::get_error_code(const nlohmann::json& in_body) {
  if (!in_body.is_object() || !in_body.contains("error")) return {};
  const auto& l_err = in_body.at("error");
  if (l_err.is_string()) return l_err.get<std::string>();
  if (!l_err.is_object()) return {};
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
  if (l_out.contains("size") && !l_out.contains("aspectRatio")) l_out["aspectRatio"] = l_out.at("size");
  l_out.erase("size");

  if (l_out.contains("content") && !l_out.contains("images")) {
    if (const auto& l_content = l_out.at("content"); l_content.is_array()) {
      nlohmann::json l_images = nlohmann::json::array();
      for (const auto& l_item : l_content)
        if (auto l_url = image_url_of_content_item(l_item); l_url && !l_url->empty())
          l_images.push_back(std::move(*l_url));
      if (!l_images.empty()) l_out["images"] = std::move(l_images);
    }
  }
  l_out.erase("content");
  l_out.erase("input");
  return l_out;
}

apipy_image_client::edits_form_t apipy_image_client::build_edit_multipart(
    const nlohmann::json& in_apipy_body, const std::vector<edit_reference_t>& in_references
) {
  edits_form_t l_form{};
  l_form.boundary_ = make_boundary();

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
  const bool l_http_ok = in_status_code >= 200 && in_status_code < 300;
  if (!l_http_ok) return doodle::seedance2::task_status::failed;
  if (in_body.contains("error")) return doodle::seedance2::task_status::failed;
  if (!extract_image_urls(in_body).empty()) return doodle::seedance2::task_status::succeeded;
  if (!extract_b64_payload(in_body).empty()) return doodle::seedance2::task_status::succeeded;
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
  l_info.resolution_  = l_req.aspect_ratio_.empty() ? l_req.image_size_ : l_req.aspect_ratio_;
  l_info.text_prompt_ = l_req.prompt_;
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

  l_req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token_));
  l_req.set(boost::beast::http::field::accept, "application/json");
  l_req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_);
  l_req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
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

  if (l_result.data_response_.contains("usage") && l_result.data_response_.at("usage").is_object()) {
    const auto& l_usage     = l_result.data_response_.at("usage");
    l_result.input_tokens_  = l_usage.value("input_tokens", std::int64_t{0});
    l_result.output_tokens_ = l_usage.value("output_tokens", std::int64_t{0});
    l_result.total_tokens_  = l_usage.value("total_tokens", std::int64_t{0});
  }

  if (l_result.status_ != doodle::seedance2::task_status::succeeded) {
    SPDLOG_LOGGER_ERROR(
        logger_, "API易 图片生成失败: HTTP {} error_code `{}` body {}", l_res.result(), l_result.error_code_,
        l_result.data_response_.dump()
    );
    co_return l_result;
  }

  for (const auto& l_url : extract_image_urls(l_result.data_response_)) {
    l_result.result_files_.push_back(l_url);
    l_result.result_file_paths_.push_back(co_await download_raw_file(l_url));
  }
  if (l_result.result_file_paths_.empty()) {
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
