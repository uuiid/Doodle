#include "transfer_station_client.h"

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/lib_warp/boost_fmt_beast.h>

#include <fmt/format.h>

namespace doodle::http::seedance2 {

namespace {

struct model_pricing {
  std::string_view name;
  std::int64_t cost_points;
  bool charge_on_failure;    // false = 失败返还
  bool charge_on_violation;  // false = 违规返还
};

// 数据来源: https://grsai.com/zh/dashboard/models
// 所有图片模型均为按次计费, 失败/违规均返还积分
// 未在页面列出的模型按同系列最近模型估算
constexpr model_pricing g_model_pricings[] = {
    // ─── nano-banana 系列 ───
    {"nano-banana-fast", 440, false, false},
    {"nano-banana-2", 1200, false, false},
    {"nano-banana-2-lite", 440, false, false},
    {"nano-banana-2-cl", 6000, false, true},
    {"nano-banana-2-2k-cl", 9000, false, true},
    {"nano-banana-2-4k-cl", 13000, false, true},
    {"nano-banana-pro", 1800, false, false},
    {"nano-banana-pro-cl", 10000, false, true},
    {"nano-banana-pro-vip", 10000, false, true},
    {"nano-banana-pro-4k-vip", 18000, false, true},
    // ─── gpt-image 系列 ───
    {"gpt-image-2", 600, false, false},
    {"gpt-image-2-vip", 2000, false, false},
    {"gpt-image-2.5", 600, false, false},
    {"gpt-image-2.5-flare", 2000, false, false},
    {"gpt-image-2.5-sunburst", 2400, false, false},
};

const model_pricing* find_pricing(std::string_view in_name) {
  for (const auto& l_p : g_model_pricings)
    if (l_p.name == in_name) return &l_p;
  return nullptr;
}

}  // namespace

// ─── 工具方法 ────────────────────────────────────────────────────────────────

nlohmann::json transfer_station_client::add_ip_to_req(const nlohmann::json& in_req, std::string_view in_ip) {
  nlohmann::json l_req = in_req;

  // images: 可选数组, 元素可能是 base64(data:) 或 url
  if (l_req.contains("images") && l_req.at("images").is_array()) {
    for (auto& l_value : l_req.at("images")) {
      if (!l_value.is_string()) continue;
      auto l_url = l_value.get<std::string>();
      l_value    = fmt::format("http://{}:38192{}", in_ip, l_url);
    }
  }

  // mask: 可选字段, url 字符串
  if (l_req.contains("mask") && l_req.at("mask").is_string()) {
    auto l_mask   = l_req.at("mask").get<std::string>();
    l_req["mask"] = fmt::format("http://{}:38192{}", in_ip, l_mask);
  }

  return l_req;
}

doodle::seedance2::task_status transfer_station_client::parse_status(const nlohmann::json& in_body) {
  if (!in_body.contains("status")) return doodle::seedance2::task_status::failed;

  const auto& l_s = in_body.at("status").get_ref<const std::string&>();
  if (l_s == "running") return doodle::seedance2::task_status::running;
  if (l_s == "succeeded") return doodle::seedance2::task_status::succeeded;
  return doodle::seedance2::task_status::failed;  // violation / failed / unknown
}

// ─── 子类方法 ─────────────────────────────────────────────────────────────────

ai_client_base::request_info_t transfer_station_client::collect_request_info(const nlohmann::json& in_request) const {
  request_info_t l_info{};
  if (in_request.contains("model")) l_info.model_ = in_request.at("model").get<std::string>();
  // nano-banana 系列用 imageSize(1K/2K/4K) 表示分辨率;
  // gpt-image 系列没有 imageSize, 用 aspectRatio(比例或像素值, 如 "16:9" / "1024x1024")
  if (in_request.contains("imageSize"))
    l_info.resolution_ = in_request.at("imageSize").get<std::string>();
  else if (in_request.contains("aspectRatio"))
    l_info.resolution_ = in_request.at("aspectRatio").get<std::string>();
  if (in_request.contains("prompt")) l_info.text_prompt_ = in_request.at("prompt").get<std::string>();
  return l_info;
}

boost::asio::awaitable<ai_client_base::run_task_result_t> transfer_station_client::run_task(
    const nlohmann::json& in_task
) {
  auto l_ip               = co_await get_ip_str();
  auto l_req_body         = add_ip_to_req(in_task, l_ip);
  l_req_body["replyType"] = "async";

  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::post, "/v1/api/generate", 11
  };
  req.body() = l_req_body.dump();
  req.set(boost::beast::http::field::content_type, "application/json");
  req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token_));
  req.set(boost::beast::http::field::accept, "application/json");
  req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_);
  req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
  http_client_ptr_->set_timeout(300s);

  boost::beast::http::response<http::basic_json_body> l_res{};
  try {
    co_await http_client_ptr_->read_and_write(req, l_res, boost::asio::use_awaitable);
  } catch (...) {
    throw_exception(
        http_request_error{
            boost::beast::http::status::internal_server_error, boost::current_exception_diagnostic_information()
        }
    );
  }

  run_task_result_t l_result;
  l_result.data_response_ = l_res.body();
  l_result.task_id_       = l_result.data_response_.at("id").get<std::string>();
  // HTTP 200 → 异步提交成功；400 同样有 id 字段
  if (l_res.result() == boost::beast::http::status::ok && l_result.data_response_.contains("id") &&
      !l_result.data_response_.at("id").get<std::string>().empty()) {
    l_result.status_ = doodle::seedance2::task_status::queued;
  } else {
    l_result.status_ = doodle::seedance2::task_status::failed;
  }
  co_return l_result;
}

boost::asio::awaitable<ai_client_base::query_task_result_t> transfer_station_client::query_task(
    const doodle::seedance2::task& in_task
) {
  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::get, fmt::format("/v1/api/result?id={}", in_task.task_id_), 11
  };
  req.set(boost::beast::http::field::content_type, "application/json");
  req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token_));
  req.set(boost::beast::http::field::accept, "application/json");
  req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_);
  req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");

  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_ptr_->read_and_write(req, l_res, boost::asio::use_awaitable);
  DOODLE_CHICK(
      l_res.result() == boost::beast::http::status::ok, "query_task error: {} {}", l_res.result(), l_res.body().dump()
  );

  query_task_result_t l_result;
  l_result.client_ptr_    = shared_from_this();
  l_result.data_response_ = l_res.body();

  // 按模型定价扣除积分
  auto l_model            = in_task.data_request_.value("model", "");
  const auto* l_pricing   = find_pricing(l_model);
  auto l_raw_status       = l_result.data_response_.value("status", "");

  if (l_raw_status == "succeeded") {
    l_result.completion_tokens_ = l_pricing ? l_pricing->cost_points : 0;
  } else if (l_raw_status == "failed") {
    l_result.completion_tokens_ = (l_pricing && l_pricing->charge_on_failure) ? l_pricing->cost_points : 0;
  } else if (l_raw_status == "violation") {
    l_result.completion_tokens_ = (l_pricing && l_pricing->charge_on_violation) ? l_pricing->cost_points : 0;
  } else {
    l_result.completion_tokens_ = 0;  // running / 未知, 暂不扣
  }

  if (!l_pricing) {
    logger_->warn("未找到模型定价: {}", l_model);
  }

  l_result.status_ = parse_status(l_result.data_response_);

  if (l_result.data_response_.contains("results") && l_result.data_response_.at("results").is_array()) {
    for (const auto& l_item : l_result.data_response_.at("results")) {
      if (l_item.contains("url") && l_item.at("url").is_string())
        l_result.result_files_.push_back(l_item.at("url").get<std::string>());
    }
  }

  co_return l_result;
}
// 不支持
boost::asio::awaitable<void> transfer_station_client::cancel_task(const std::string& in_task_id) {
  throw_exception(doodle_error{"transfer_station_client::cancel_task not supported"});
  co_return;
}

boost::asio::awaitable<void> transfer_station_client::download_result(query_task_result_t* in_data) {
  for (auto& l_url : in_data->result_files_) {
    auto l_file = co_await download_raw_file(l_url);
    in_data->result_file_paths_.push_back(l_file);
  }
}

}  // namespace doodle::http::seedance2