#include "kmt_image_client.h"

#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/kitsu_ctx_t.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/boost_fmt_beast.h>
#include <doodle_lib/logger/logger.h>

#include <boost/system/system_error.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fmt/format.h>
#include <fstream>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace doodle::http::seedance2 {

namespace {

constexpr std::string_view g_generations_path{"/v1/images/generations"};
constexpr std::string_view g_edits_path{"/v1/images/edits"};
constexpr std::string_view g_tasks_prefix{"/v1/images/tasks/"};
constexpr std::string_view g_pictures_prefix{"/api/seedance2/pictures/"};
constexpr std::string_view g_edit_image_field{"image"};
constexpr std::string_view g_compressed_ext{".png"};
constexpr std::string_view g_compressed_mime{"image/png"};
constexpr std::chrono::seconds g_request_timeout{300};
constexpr std::size_t g_max_reference_images{16};
constexpr std::int32_t g_max_reference_image_side{2048};
constexpr std::int64_t g_kmt_default_consumed_tokens{5000};

struct model_pricing {
  std::string_view name;
  std::int64_t cost_points;
  bool charge_on_failure;
  bool charge_on_violation;
};

// 数据来源: https://docs.52pay.com/advanced/models#%E5%9B%BE%E5%83%8F%E6%A8%A1%E5%9E%8B
// 这里登记的是允许提交的模型, 计费口径与 transfer_station 相同: 按次一口价, 失败/违规不扣费。
constexpr model_pricing g_model_pricings[] = {
    {"gpt-image-2", 600, false, false},
    {"gpt-image-2.5", 600, false, false},
    {"gpt-image-2.5-flare", 2000, false, false},
    {"gpt-image-2.5-sunburst", 2400, false, false},
};

const model_pricing* find_pricing(std::string_view in_name) {
  for (const auto& l_p : g_model_pricings)
    if (l_p.name == in_name) return &l_p;
  return nullptr;
}

FSys::path parse_site_image_id(std::string_view in_url) {
  DOODLE_CHICK(in_url.starts_with(g_pictures_prefix), "URL 不以 {} 开头: {}", g_pictures_prefix, in_url);
  FSys::path l_name{in_url};
  auto l_ext    = l_name.extension().generic_string();
  auto l_id_str = l_name.stem().generic_string();

  uuid l_id     = from_uuid_str(l_id_str);
  return g_ctx().get<kitsu_ctx_t>().get_sd2_pictures_file(l_id, l_ext);
}

kmt_image_client::reference_image_t compress_reference_image(const FSys::path& in_path) {
  cv::Mat l_image = cv::imread(in_path.generic_string(), cv::IMREAD_UNCHANGED);
  DOODLE_CHICK(!l_image.empty(), "参考图解码失败: {}", in_path.generic_string());

  if (const auto l_long_side = std::max(l_image.cols, l_image.rows); l_long_side > g_max_reference_image_side) {
    const auto l_scale  = static_cast<double>(g_max_reference_image_side) / static_cast<double>(l_long_side);
    const auto l_width  = std::max(1, static_cast<int>(std::lround(static_cast<double>(l_image.cols) * l_scale)));
    const auto l_height = std::max(1, static_cast<int>(std::lround(static_cast<double>(l_image.rows) * l_scale)));
    cv::resize(l_image, l_image, cv::Size{l_width, l_height}, 0.0, 0.0, cv::INTER_AREA);
  }

  std::vector<uchar> l_buffer{};
  DOODLE_CHICK(
      cv::imencode(std::string{g_compressed_ext}, l_image, l_buffer, {cv::IMWRITE_PNG_COMPRESSION, 9}) &&
          !l_buffer.empty(),
      "参考图编码失败: {}", in_path.generic_string()
  );

  return kmt_image_client::reference_image_t{
      .filename_ = in_path.stem().generic_string() + std::string{g_compressed_ext},
      .bytes_    = std::string{l_buffer.begin(), l_buffer.end()},
      .cols_     = l_image.cols,
      .rows_     = l_image.rows
  };
}

std::string model_of_task(const doodle::seedance2::task& in_task) {
  const auto& l_req = in_task.data_request_;
  if (!l_req.is_object() || !l_req.contains("model") || !l_req.at("model").is_string()) return {};
  return l_req.at("model").get<std::string>();
}

}  // namespace

kmt_image_client::request_t::request_t(const doodle::seedance2::transfer_station_request_t& in_req)
    : model_(in_req.model_),
      prompt_(in_req.prompt_),
      size_(in_req.image_size_.empty() ? in_req.aspect_ratio_ : in_req.image_size_),
      quality_(in_req.quality_) {
  for (const auto& l_url : in_req.reference_images_)
    reference_images_.push_back(compress_reference_image(parse_site_image_id(l_url)));
}

void to_json(nlohmann::json& j, const kmt_image_client::request_t& in_req) {
  j["model"]           = in_req.model_;
  j["prompt"]          = in_req.prompt_;
  j["size"]            = in_req.size_;
  j["async"]           = true;
  j["response_format"] = "url";
  if (!in_req.quality_.empty()) j["quality"] = in_req.quality_;
}

kmt_image_client::edits_form_t kmt_image_client::request_t::to_multipart() const {
  edits_form_t l_form{};
  l_form.boundary_         = "----doodle" + boost::uuids::to_string(core_set::get_set().get_uuid());

  const auto l_append_text = [&l_form](std::string_view in_key, std::string_view in_value) {
    if (in_value.empty()) return;
    l_form.body_ += "--";
    l_form.body_ += l_form.boundary_;
    l_form.body_ += "\r\nContent-Disposition: form-data; name=\"";
    l_form.body_ += in_key;
    l_form.body_ += "\"\r\n\r\n";
    l_form.body_ += in_value;
    l_form.body_ += "\r\n";
  };

  l_append_text("model", model_);
  l_append_text("prompt", prompt_);
  l_append_text("size", size_);
  l_append_text("quality", quality_);
  l_append_text("async", "true");
  l_append_text("response_format", "url");

  for (const auto& l_image : reference_images_) {
    SPDLOG_LOGGER_INFO(
        g_logger_ctrl().get_http(), "参考图已转码为 PNG: {} ({}x{}), {} 字节", l_image.filename_, l_image.cols_,
        l_image.rows_, l_image.bytes_.size()
    );

    l_form.body_ += "--";
    l_form.body_ += l_form.boundary_;
    l_form.body_ += "\r\nContent-Disposition: form-data; name=\"";
    l_form.body_ += g_edit_image_field;
    l_form.body_ += "\"; filename=\"";
    l_form.body_ += l_image.filename_;
    l_form.body_ += "\"\r\nContent-Type: ";
    l_form.body_ += g_compressed_mime;
    l_form.body_ += "\r\n\r\n";
    l_form.body_ += l_image.bytes_;
    l_form.body_ += "\r\n";
  }

  l_form.body_ += "--";
  l_form.body_ += l_form.boundary_;
  l_form.body_ += "--\r\n";
  return l_form;
}

void from_json(const nlohmann::json& j, kmt_image_client::response_t& in) {
  in.id_     = j.value("id", std::string{});
  in.object_ = j.value("object", std::string{});
  in.status_ = j.value("status", std::string{});
  in.model_  = j.value("model", std::string{});
  in.created_ = j.value("created", std::int64_t{0});
  if (!j.contains("data") || !j.at("data").is_array()) return;
  for (const auto& l_item : j.at("data"))
    if (l_item.is_object()) in.data_.push_back(l_item.get<kmt_image_client::data_item_t>());
}

doodle::seedance2::task_status kmt_image_client::parse_status(std::string_view in_status) {
  if (in_status == "queued" || in_status == "in_progress") return doodle::seedance2::task_status::running;
  if (in_status == "completed") return doodle::seedance2::task_status::succeeded;
  if (in_status == "failed") return doodle::seedance2::task_status::failed;
  if (in_status == "violation") return doodle::seedance2::task_status::violation;
  throw_exception(doodle_error{fmt::format("kmt_image_client: 未识别的任务状态 {}", in_status)});
}

std::string kmt_image_client::parse_task_id(const nlohmann::json& in_body) {
  if (!in_body.is_object()) return {};
  for (const auto* l_key : {"id", "task_id"})
    if (in_body.contains(l_key) && in_body.at(l_key).is_string() && !in_body.at(l_key).get<std::string>().empty())
      return in_body.at(l_key).get<std::string>();
  if (in_body.contains("data") && in_body.at("data").is_object())
    for (const auto* l_key : {"id", "task_id"})
      if (in_body.at("data").contains(l_key) && in_body.at("data").at(l_key).is_string() &&
          !in_body.at("data").at(l_key).get<std::string>().empty())
        return in_body.at("data").at(l_key).get<std::string>();
  return {};
}

std::optional<kmt_image_client::response_t> kmt_image_client::parse_query_response(const nlohmann::json& in_body) {
  if (!in_body.is_object()) return std::nullopt;
  return in_body.get<response_t>();
}

ai_client_base::request_info_t kmt_image_client::collect_request_info(const nlohmann::json& in_request) const {
  const auto l_req = in_request.get<doodle::seedance2::transfer_station_request_t>();

  DOODLE_CHICK_HTTP(
      l_req.reference_images_.size() <= g_max_reference_images, bad_request, "参考图最多 {} 张, 收到 {} 张",
      g_max_reference_images, l_req.reference_images_.size()
  );

  DOODLE_CHICK_HTTP(find_pricing(l_req.model_) != nullptr, bad_request, "模型 {} 未登记定价, 不允许提交", l_req.model_);

  request_info_t l_info{};
  l_info.model_       = l_req.model_;
  l_info.resolution_  = l_req.image_size_.empty() ? l_req.aspect_ratio_ : l_req.image_size_;
  l_info.text_prompt_ = l_req.prompt_;
  for (auto&& l_url : l_req.reference_images_) parse_site_image_id(l_url);

  DOODLE_CHICK_HTTP(!l_info.model_.empty(), bad_request, "缺少模型名称");
  DOODLE_CHICK_HTTP(!l_info.resolution_.empty(), bad_request, "缺少分辨率");
  DOODLE_CHICK_HTTP(!l_info.text_prompt_.empty(), bad_request, "缺少提示词");
  return l_info;
}

boost::asio::awaitable<ai_client_base::run_task_result_t> kmt_image_client::submit_task(
    const doodle::seedance2::task& in_task
) {
  run_task_result_t l_result{};
  l_result.status_ = doodle::seedance2::task_status::failed;

  if (in_task.type_ == doodle::seedance2::task_type::inference) {
    logger_->error("快米兔 图片后端不支持推理任务");
    l_result.data_response_ = nlohmann::json{{"error", "快米兔 图片后端不支持推理任务"}};
    co_return l_result;
  }
  if (in_task.type_ == doodle::seedance2::task_type::video) {
    logger_->error("快米兔 图片后端不支持视频任务");
    l_result.data_response_ = nlohmann::json{{"error", "快米兔 图片后端不支持视频任务"}};
    co_return l_result;
  }

  auto l_req_body = in_task.data_request_.get<doodle::seedance2::transfer_station_request_t>();

  const kmt_image_client::request_t l_kmt_req{l_req_body};

  boost::beast::http::request<boost::beast::http::string_body> l_req{boost::beast::http::verb::post, {}, 11};
  if (l_kmt_req.reference_images_.empty()) {
    l_req.target(std::string{g_generations_path});
    l_req.body() = (nlohmann::json{} = l_kmt_req).dump();
    l_req.set(boost::beast::http::field::content_type, "application/json");
  } else {
    auto l_form = l_kmt_req.to_multipart();
    l_req.target(std::string{g_edits_path});
    l_req.body() = std::move(l_form.body_);
    l_req.set(boost::beast::http::field::content_type, l_form.content_type());
  }

  l_req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token_));
  l_req.set(boost::beast::http::field::accept, "application/json");
  l_req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_);
  l_req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
  http_client_ptr_->set_timeout(g_request_timeout);

  boost::beast::http::response<http::basic_json_body> l_res{};
  try {
    co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  } catch (const boost::system::system_error& e) {
    SPDLOG_LOGGER_ERROR(logger_, "快米兔 图片提交连接失败: {}", e.what());
    l_result.status_        = doodle::seedance2::task_status::failed;
    l_result.need_retry_    = true;
    l_result.data_response_ = nlohmann::json{{"error", e.what()}};
    co_return l_result;
  }

  l_result.data_response_ = l_res.body();
  l_result.task_id_       = parse_task_id(l_result.data_response_);

  if (l_res.result() == boost::beast::http::status::too_many_requests ||
      l_res.result() == boost::beast::http::status::internal_server_error ||
      l_res.result() == boost::beast::http::status::service_unavailable ||
      l_res.result() == boost::beast::http::status::bad_gateway ||
      l_res.result() == boost::beast::http::status::gateway_timeout)
    l_result.need_retry_ = true;

  if (l_res.result() == boost::beast::http::status::ok && !l_result.task_id_.empty()) {
    l_result.status_ = doodle::seedance2::task_status::queued;
  } else {
    l_result.status_ = doodle::seedance2::task_status::failed;
    SPDLOG_LOGGER_ERROR(
        logger_, "快米兔 图片提交失败: HTTP {} body {}", l_res.result(), l_result.data_response_.dump()
    );
  }
  co_return l_result;
}

boost::asio::awaitable<ai_client_base::run_task_result_t> kmt_image_client::query_task(
    const doodle::seedance2::task& in_task
) {
  run_task_result_t l_result{};
  l_result.status_ = doodle::seedance2::task_status::failed;
  if (in_task.task_id_.empty()) {
    l_result.data_response_ = nlohmann::json{{"error", "task id 为空, 无法查询"}};
    co_return l_result;
  }

  boost::beast::http::request<boost::beast::http::string_body> l_req{
      boost::beast::http::verb::get, fmt::format("{}{}", g_tasks_prefix, in_task.task_id_), 11
  };
  l_req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token_));
  l_req.set(boost::beast::http::field::accept, "application/json");
  l_req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_);
  l_req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");

  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_ptr_->read_and_write(l_req, l_res, boost::asio::use_awaitable);
  DOODLE_CHICK(
      l_res.result() == boost::beast::http::status::ok, "kmt query_task error: {} {}", l_res.result(),
      l_res.body().dump()
  );

  l_result.data_response_ = l_res.body();
  l_result.task_id_       = in_task.task_id_;
  auto l_req_body         = in_task.data_request_.get<doodle::seedance2::transfer_station_request_t>();

  const auto l_resp = parse_query_response(l_result.data_response_);
  if (!l_resp) {
    logger_->error("快米兔 图片任务 {} 响应不是对象: {}", in_task.task_id_, l_result.data_response_.dump());
    co_return l_result;
  }
  l_result.status_ = parse_status(l_resp->status_);

  const auto* l_pricing   = find_pricing(l_req_body.model_);
  switch (l_result.status_) {
    case doodle::seedance2::task_status::succeeded:
      l_result.completion_tokens_ = l_pricing ? l_pricing->cost_points : 0;
      break;
    case doodle::seedance2::task_status::failed:
      l_result.completion_tokens_ = (l_pricing && l_pricing->charge_on_failure) ? l_pricing->cost_points : 0;
      break;
    case doodle::seedance2::task_status::violation:
      l_result.completion_tokens_ = (l_pricing && l_pricing->charge_on_violation) ? l_pricing->cost_points : 0;
      break;
    default:
      l_result.completion_tokens_ = 0;
      break;
  }

  if (!l_pricing) logger_->warn("未找到模型定价: {}", model_of_task(in_task));

  if (l_result.status_ == doodle::seedance2::task_status::succeeded) {
    for (const auto& l_item : l_resp->data_)
      if (!l_item.url_.empty()) l_result.result_files_.push_back(l_item.url_);

    if (l_result.result_files_.empty()) {
      logger_->error("快米兔 图片任务 {} 完成但响应里没有图片 url", in_task.task_id_);
      l_result.status_            = doodle::seedance2::task_status::failed;
      l_result.completion_tokens_ = 0;
    }
  }

  co_return l_result;
}

boost::asio::awaitable<void> kmt_image_client::cancel_task(const std::string& in_task_id) {
  throw_exception(doodle_error{"kmt_image_client::cancel_task not supported"});
  co_return;
}

boost::asio::awaitable<void> kmt_image_client::download_result(run_task_result_t& in_data) {
  for (auto& l_url : in_data.result_files_) {
    auto l_file = co_await download_raw_file(l_url);
    in_data.result_file_paths_.push_back(l_file);
  }
}

std::size_t kmt_image_client::default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const {
  switch (in_task_type) {
    case doodle::seedance2::task_type::video:
      return doodle_config::g_max_task_completion_tokens;
    case doodle::seedance2::task_type::inference:
      return doodle_config::g_ark_inference_default_consumed_tokens;
    default:
      return static_cast<std::size_t>(g_kmt_default_consumed_tokens);
  }
}

}  // namespace doodle::http::seedance2
