#include "transfer_station_client.h"

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/lib_warp/boost_fmt_beast.h>

#include <boost/uuid/uuid_io.hpp>

#include <fmt/format.h>

namespace doodle::http {

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
    {"nano-banana-2.1", 1200, false, false},
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

doodle::seedance2::transfer_station_request_t transfer_station_client::add_ip_to_req(
    const doodle::seedance2::transfer_station_request_t& in_req, std::string_view in_ip, std::string_view in_secret,
    std::string_view in_uid
) {
  doodle::seedance2::transfer_station_request_t l_req = in_req;

  // images: 每项都可能是 base64(data:) 或 url, 也可能是本站路径。
  // 这里过去对元素**无条件**加前缀, 会把已经是绝对 URL 的元素(对象存储签名 URL 等)拼坏,
  // 现在统一走 to_public_url: 绝对 URL 原样保留, 站点内路径补前缀并签名。
  for (auto& l_value : l_req.reference_images_) l_value = to_public_url(l_value, in_ip, in_secret, in_uid);

  // mask: 同一个改写规则
  if (!l_req.mask_.empty()) l_req.mask_ = to_public_url(l_req.mask_, in_ip, in_secret, in_uid);

  return l_req;
}

doodle::seedance2::task_status transfer_station_client::parse_status(const nlohmann::json& in_body) {
  if (!in_body.contains("status")) return doodle::seedance2::task_status::failed;

  const auto& l_s = in_body.at("status").get_ref<const std::string&>();
  if (l_s == "running") return doodle::seedance2::task_status::running;
  if (l_s == "succeeded") return doodle::seedance2::task_status::succeeded;
  // 违规单独成一个状态, 不能并进 failed: 它的计费规则与失败不同
  // (见 query_task 中的 charge_on_violation), 合并会产生「status=failed 却扣了费」的记录
  if (l_s == "violation") return doodle::seedance2::task_status::violation;
  return doodle::seedance2::task_status::failed;  // failed / unknown
}

// ─── 子类方法 ─────────────────────────────────────────────────────────────────

ai_client_base::request_info_t transfer_station_client::collect_request_info(const nlohmann::json& in_request) const {
  // 基类签名收的是裸 json(它同时服务方舟/推理等其它形状的后端), 所以这里显式走一次
  // 类型化读法: 键名/取值域的判据全部落在 transfer_station_request_t 上, 不再在本函数里
  // 裸读字符串。from_json 是宽容的(见 image_request.h), 形状不对时得到的是空结构体。
  const auto l_req = in_request.get<doodle::seedance2::transfer_station_request_t>();

  request_info_t l_info{};
  l_info.model_       = l_req.model_;
  // nano-banana 系列用 imageSize(1K/2K/4K) 表示分辨率;
  // gpt-image 系列没有 imageSize, 用 aspectRatio(比例或像素值, 如 "16:9" / "1024x1024")
  l_info.resolution_  = l_req.image_size_.empty() ? l_req.aspect_ratio_ : l_req.image_size_;
  l_info.text_prompt_ = l_req.prompt_;
  return l_info;
}

boost::asio::awaitable<ai_client_base::run_task_result_t> transfer_station_client::submit_task(
    const doodle::seedance2::task& in_task
) {
  // 中转站只做图片/视频生成, 不支持同步推理
  if (in_task.type_ == doodle::seedance2::task_type::inference) {
    logger_->error("中转站后端不支持推理任务");
    run_task_result_t l_result{};
    l_result.status_        = doodle::seedance2::task_status::failed;
    l_result.data_response_ = nlohmann::json{{"error", "中转站后端不支持推理任务"}};
    co_return l_result;
  }

  // 数据请求按中转站的请求形状解析后使用: 定价校验、素材 URL 改写、replyType 覆盖
  // 都作用在这个有类型的结构体上, 最后一次性 dump 成请求体。
  auto l_req_body = in_task.data_request_.get<doodle::seedance2::transfer_station_request_t>();

  // 未登记定价的模型不允许提交, 否则会按 0 积分计费
  if (find_pricing(l_req_body.model_) == nullptr) {
    logger_->error("模型 {} 未登记定价, 拒绝提交", l_req_body.model_);
    run_task_result_t l_result{};
    l_result.status_ = doodle::seedance2::task_status::failed;
    l_result.data_response_ =
        nlohmann::json{{"error", fmt::format("模型 {} 未登记定价, 不允许提交", l_req_body.model_)}};
    co_return l_result;
  }

  auto l_ip  = co_await get_ip_str();
  l_req_body = add_ip_to_req(
      l_req_body, l_ip, url_sign_secret_,
      in_task.user_id_.is_nil() ? std::string{} : boost::uuids::to_string(in_task.user_id_)
  );
  // 只有异步形态才拿得到 id 去轮询 /v1/api/result
  l_req_body.reply_type_ = "async";

  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::post, "/v1/api/generate", 11
  };
  req.body() = (nlohmann::json{} = l_req_body).dump();
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

    if (l_result.data_response_.contains("error") && l_result.data_response_.at("error").is_string() &&
        (l_result.data_response_.at("error").get_ref<const std::string&>().starts_with("generate failed") ||
         l_result.data_response_.at("error").get_ref<const std::string&>().starts_with("generate image failed") ||
         l_result.data_response_.at("error").get_ref<const std::string&>().starts_with(
             "The model load is too high, please try again later"
         ) ||
         l_result.data_response_.at("error").get_ref<const std::string&>().starts_with("image upload failed") ||
         l_result.data_response_.at("error").get_ref<const std::string&>().starts_with("server_timeout")))
      l_result.need_retry_ = true;
  }
  co_return l_result;
}

boost::asio::awaitable<ai_client_base::run_task_result_t> transfer_station_client::query_task(
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

  run_task_result_t l_result;
  l_result.data_response_ = l_res.body();

  // 按模型定价扣除积分
  auto l_model            = in_task.data_request_.value("model", "");
  const auto* l_pricing   = find_pricing(l_model);
  l_result.status_        = parse_status(l_result.data_response_);

  if (l_result.status_ == doodle::seedance2::task_status::succeeded) {
    l_result.completion_tokens_ = l_pricing ? l_pricing->cost_points : 0;
  } else if (l_result.status_ == doodle::seedance2::task_status::failed) {
    l_result.completion_tokens_ = (l_pricing && l_pricing->charge_on_failure) ? l_pricing->cost_points : 0;
  } else if (l_result.status_ == doodle::seedance2::task_status::violation) {
    l_result.completion_tokens_ = (l_pricing && l_pricing->charge_on_violation) ? l_pricing->cost_points : 0;
  } else {
    l_result.completion_tokens_ = 0;  // running / 未知, 暂不扣
  }

  if (!l_pricing) {
    logger_->warn("未找到模型定价: {}", l_model);
  }
  if (l_result.data_response_.contains("results") && l_result.data_response_.at("results").is_array()) {
    for (const auto& l_item : l_result.data_response_.at("results")) {
      if (l_item.contains("url") && l_item.at("url").is_string())
        l_result.result_files_.push_back(l_item.at("url").get<std::string>());
    }
  }

  l_result.need_retry_ =
      l_result.status_ == doodle::seedance2::task_status::failed && l_result.data_response_.contains("error") &&
      l_result.data_response_.at("error").is_string() &&
      (l_result.data_response_.at("error").get_ref<const std::string&>().starts_with("generate failed") ||
       l_result.data_response_.at("error").get_ref<const std::string&>().starts_with("generate image failed") ||
       l_result.data_response_.at("error").get_ref<const std::string&>().starts_with(
           "The model load is too high, please try again later"
       ) ||
       l_result.data_response_.at("error").get_ref<const std::string&>().starts_with("image upload failed") ||
       l_result.data_response_.at("error").get_ref<const std::string&>().starts_with("server_timeout"));
  co_return l_result;
}
// 不支持
boost::asio::awaitable<void> transfer_station_client::cancel_task(const std::string& in_task_id) {
  throw_exception(doodle_error{"transfer_station_client::cancel_task not supported"});
  co_return;
}

boost::asio::awaitable<void> transfer_station_client::download_result(run_task_result_t& in_data) {
  for (auto& l_url : in_data.result_files_) {
    auto l_file = co_await download_raw_file(l_url);
    in_data.result_file_paths_.push_back(l_file);
  }
}
std::size_t transfer_station_client::default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const {
  switch (in_task_type) {
    case doodle::seedance2::task_type::video:
      return doodle_config::g_max_task_completion_tokens;
    case doodle::seedance2::task_type::inference:
      // 中转站后端不支持推理, 走到这里说明路由配置错了
      return doodle_config::g_ark_inference_default_consumed_tokens;
    default:
      return 5000;
  }
}
}  // namespace doodle::http