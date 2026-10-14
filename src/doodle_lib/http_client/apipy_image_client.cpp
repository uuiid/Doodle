#include "apipy_image_client.h"

#include <doodle_lib/core/base64.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/lib_warp/boost_fmt_beast.h>

#include <boost/uuid/uuid_io.hpp>
#include <fmt/format.h>

#include <fstream>

namespace doodle::http::seedance2 {

namespace {

/// 生成接口路径(文生图)
constexpr std::string_view g_generations_path{"/v1/images/generations"};
/// 图片编辑接口路径(上传参考图改图 / 多图融合)。
/// 本客户端目前只走生成接口: edits 是 multipart/form-data, 需要换一种请求体类型,
/// 留到真正接入那个场景时再加, 不预先写一条没有调用者的分支。
constexpr std::string_view g_edits_path{"/v1/images/edits"};

/// b64_json 落盘时用的扩展名。模型固定输出 PNG。
constexpr std::string_view g_default_ext{".png"};

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

  auto l_path = core_set::get_set().get_cache_root("http") /
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

// ─── 子类方法 ────────────────────────────────────────────────────────────────

ai_client_base::request_info_t apipy_image_client::collect_request_info(const nlohmann::json& in_request) const {
  request_info_t l_info{};
  if (in_request.contains("model") && in_request.at("model").is_string())
    l_info.model_ = in_request.at("model").get<std::string>();
  if (l_info.model_.empty()) l_info.model_ = std::string{g_default_model};
  // 本后端用 size(宽x高, 如 "2048x1152") 表示分辨率; 不传等同 auto
  if (in_request.contains("size") && in_request.at("size").is_string())
    l_info.resolution_ = in_request.at("size").get<std::string>();
  if (in_request.contains("prompt") && in_request.at("prompt").is_string())
    l_info.text_prompt_ = in_request.at("prompt").get<std::string>();
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

  // 请求体定稿。前端把该模型的请求体直接塞在 data_request_ 里(扁平信封), 这里只做两件事:
  //   1) 补默认 model; 2) 显式要求 URL 输出。
  nlohmann::json l_body = in_task.data_request_;
  if (!l_body.is_object()) l_body = nlohmann::json::object();
  if (!l_body.contains("model") || l_body.value("model", std::string{}).empty())
    l_body["model"] = std::string{g_default_model};

  // 为什么显式要 url 而不是吃默认的 base64: 默认返回的 b64_json 是数 MB 的字符串,
  // 它会一路经 run_task_result_t::data_response_ 落进 sd2::task::data_response_ 存库,
  // 把 sqlite 行撑到几 MB; 而且这段 base64 对调用方毫无用处(它要的是文件)。
  // 要 URL 则复用基类现成的 download_raw_file(带 3 次重试 + 落缓存目录)。
  // 官方说明: response_format 只有 -All / -VIP 支持, 官转传了直接 400 —— 本后端接的
  // 正是 -VIP 系列, 所以这里可以放心传。
  // 注意 data[0] 里 url 与 b64_json 只会出现其一; 若令牌分组不是 image2_OSS 而退回了
  // base64, 下面的分支会照样把它解出来落盘, 不会失败。
  l_body["response_format"] = "url";

  auto l_ip = co_await get_ip_str();
  l_body    = add_ip_to_req(
      l_body, l_ip, url_sign_secret_, in_task.user_id_.is_nil() ? std::string{} : boost::uuids::to_string(in_task.user_id_)
  );

  boost::beast::http::request<boost::beast::http::string_body> l_req{
      boost::beast::http::verb::post, std::string{g_generations_path}, 11
  };
  l_req.body() = l_body.dump();
  l_req.set(boost::beast::http::field::content_type, "application/json");
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
    const auto& l_usage  = l_result.data_response_.at("usage");
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
