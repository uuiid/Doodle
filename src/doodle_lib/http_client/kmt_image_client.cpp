#include "kmt_image_client.h"

#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/kitsu_ctx_t.h>

#include <doodle_lib/core/base64.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/boost_fmt_beast.h>

#include <boost/system/system_error.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <fmt/format.h>
#include <fstream>

namespace doodle::http::seedance2 {

namespace {

constexpr std::string_view g_generations_path{"/v1/images/generations"};
constexpr std::string_view g_edits_path{"/v1/images/edits"};
constexpr std::string_view g_tasks_prefix{"/v1/images/tasks/"};
constexpr std::string_view g_default_ext{".png"};
constexpr std::chrono::seconds g_request_timeout{300};
constexpr std::size_t g_max_reference_images{16};
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

struct edits_form_t {
  std::string boundary_;
  std::string body_;

  [[nodiscard]] std::string content_type() const { return "multipart/form-data; boundary=" + boundary_; }
};

edits_form_t to_multipart(const kmt_image_client::request_t& in_req) {
  edits_form_t l_form{};
  l_form.boundary_ = "----doodle" + boost::uuids::to_string(core_set::get_set().get_uuid());

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

  l_append_text("model", in_req.model_);
  l_append_text("prompt", in_req.prompt_);
  l_append_text("size", in_req.size_);
  l_append_text("quality", in_req.quality_);
  l_append_text("async", "true");
  l_append_text("response_format", "url");

  for (const auto& l_url : in_req.image_urls_) {
    if (l_url.empty()) continue;
    l_form.body_ += "--";
    l_form.body_ += l_form.boundary_;
    l_form.body_ += "\r\nContent-Disposition: form-data; name=\"image\"\r\n\r\n";
    l_form.body_ += l_url;
    l_form.body_ += "\r\n";
  }

  l_form.body_ += "--";
  l_form.body_ += l_form.boundary_;
  l_form.body_ += "--\r\n";
  return l_form;
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
      quality_(in_req.quality_),
      image_urls_(in_req.reference_images_) {}

void to_json(nlohmann::json& j, const kmt_image_client::request_t& in_req) {
  j["model"]           = in_req.model_;
  j["prompt"]          = in_req.prompt_;
  j["size"]            = in_req.size_;
  j["async"]           = true;
  j["response_format"] = "url";
  if (!in_req.quality_.empty()) j["quality"] = in_req.quality_;
}

doodle::seedance2::transfer_station_request_t kmt_image_client::add_ip_to_req(
    const doodle::seedance2::transfer_station_request_t& in_req, std::string_view in_ip, std::string_view in_secret,
    std::string_view in_uid
) {
  doodle::seedance2::transfer_station_request_t l_req = in_req;
  for (auto& l_value : l_req.reference_images_) l_value = to_public_url(l_value, in_ip, in_secret, in_uid);
  if (!l_req.mask_.empty()) l_req.mask_ = to_public_url(l_req.mask_, in_ip, in_secret, in_uid);
  return l_req;
}

doodle::seedance2::task_status kmt_image_client::parse_status(const nlohmann::json& in_body) {
  if (!in_body.is_object() || !in_body.contains("status") || !in_body.at("status").is_string())
    return doodle::seedance2::task_status::failed;

  const auto& l_s = in_body.at("status").get_ref<const std::string&>();
  if (l_s == "queued" || l_s == "in_progress" || l_s == "pending" || l_s == "running")
    return doodle::seedance2::task_status::running;
  if (l_s == "completed" || l_s == "succeeded") return doodle::seedance2::task_status::succeeded;
  if (l_s == "cancelled") return doodle::seedance2::task_status::cancelled;
  // 违规单独成一个状态, 不能并进 failed: 它的计费规则与失败不同
  if (l_s == "violation") return doodle::seedance2::task_status::violation;
  if (l_s == "failed" || l_s == "error") return doodle::seedance2::task_status::failed;
  return doodle::seedance2::task_status::failed;
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

nlohmann::json kmt_image_client::strip_b64_payload(nlohmann::json in_body) {
  if (!in_body.is_object() || !in_body.contains("data")) return in_body;
  auto& l_data = in_body.at("data");
  if (!l_data.is_array()) return in_body;
  for (auto& l_item : l_data)
    if (l_item.is_object()) l_item.erase("b64_json");
  return in_body;
}

ai_client_base::request_info_t kmt_image_client::collect_request_info(const nlohmann::json& in_request) const {
  const auto l_req = in_request.get<doodle::seedance2::transfer_station_request_t>();

  DOODLE_CHICK_HTTP(
      l_req.reference_images_.size() <= g_max_reference_images, bad_request, "参考图最多 {} 张, 收到 {} 张",
      g_max_reference_images, l_req.reference_images_.size()
  );

  request_info_t l_info{};
  l_info.model_       = l_req.model_;
  l_info.resolution_  = l_req.image_size_.empty() ? l_req.aspect_ratio_ : l_req.image_size_;
  l_info.text_prompt_ = l_req.prompt_;

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

  if (find_pricing(l_req_body.model_) == nullptr) {
    logger_->error("模型 {} 未登记定价, 拒绝提交", l_req_body.model_);
    l_result.data_response_ =
        nlohmann::json{{"error", fmt::format("模型 {} 未登记定价, 不允许提交", l_req_body.model_)}};
    co_return l_result;
  }

  auto l_ip  = co_await get_ip_str();
  l_req_body = add_ip_to_req(
      l_req_body, l_ip, url_sign_secret_,
      in_task.user_id_.is_nil() ? std::string{} : boost::uuids::to_string(in_task.user_id_)
  );

  const kmt_image_client::request_t l_kmt_req{l_req_body};

  boost::beast::http::request<boost::beast::http::string_body> l_req{boost::beast::http::verb::post, {}, 11};
  if (l_kmt_req.image_urls_.empty()) {
    l_req.target(std::string{g_generations_path});
    l_req.body() = (nlohmann::json{} = l_kmt_req).dump();
    l_req.set(boost::beast::http::field::content_type, "application/json");
  } else {
    auto l_form = to_multipart(l_kmt_req);
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
  l_result.status_        = parse_status(l_result.data_response_);

  const auto* l_pricing   = find_pricing(model_of_task(in_task));
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
    if (!l_result.data_response_.contains("data") || !l_result.data_response_.at("data").is_array() ||
        l_result.data_response_.at("data").empty()) {
      logger_->error("快米兔 图片任务 {} 完成但响应里没有图片数组", in_task.task_id_);
      l_result.status_            = doodle::seedance2::task_status::failed;
      l_result.completion_tokens_ = 0;
      co_return l_result;
    }
    for (const auto& l_item : l_result.data_response_.at("data")) {
      if (!l_item.is_object()) continue;
      if (l_item.contains("url") && l_item.at("url").is_string())
        l_result.result_files_.push_back(l_item.at("url").get<std::string>());
      else if (l_item.contains("b64_json") && l_item.at("b64_json").is_string())
        l_result.result_file_paths_.push_back(
            write_b64_to_cache(l_item.at("b64_json").get<std::string>(), g_default_ext)
        );
    }
    l_result.data_response_ = strip_b64_payload(std::move(l_result.data_response_));
    if (l_result.result_files_.empty() && l_result.result_file_paths_.empty()) {
      logger_->error("快米兔 图片任务 {} 完成但既没有 url 也没有 b64_json", in_task.task_id_);
      l_result.status_ = doodle::seedance2::task_status::failed;
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

FSys::path kmt_image_client::write_b64_to_cache(std::string_view in_b64, std::string_view in_ext) const {
  if (in_b64.starts_with("data:"))
    if (const auto l_comma = in_b64.find(','); l_comma != std::string_view::npos) in_b64.remove_prefix(l_comma + 1);

  auto l_path = core_set::get_set().get_cache_root("http") /
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

}  // namespace doodle::http::seedance2
