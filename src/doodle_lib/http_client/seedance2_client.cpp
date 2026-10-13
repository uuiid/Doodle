#include "seedance2_client.h"

#include "doodle_core/configure/static_value.h"
#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/boost_uuid_warp.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/ffmpeg_video.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/lib_warp/boost_fmt_beast.h>

#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <fmt/format.h>

namespace doodle::http::seedance2 {

namespace {
constexpr std::string_view g_generations_path{"/api/v3/contents/generations/tasks"};
constexpr std::string_view g_responses_path{"/api/v3/responses"};
}  // namespace

// ─── 工具方法 ────────────────────────────────────────────────────────────────

doodle::seedance2::task_status seedance2_client::parse_status(const nlohmann::json& in_body) {
  return in_body.contains("status") ? in_body.at("status").get<doodle::seedance2::task_status>()
                                    : doodle::seedance2::task_status::failed;
}

bool seedance2_client::is_timeout_error(const nlohmann::json& in_body) {
  if (!in_body.contains("error")) return false;
  auto l_message = in_body.at("error").value("message", std::string{});
  return l_message.find("timeout while fetching resource") != std::string::npos;
}

// ─── 子类方法 ─────────────────────────────────────────────────────────────────

namespace {
/// 把 Responses 的 `input` 摘要成一行可读文本, 追加到 `io_out`。
///
/// 两种形状都认: 字符串(= 一条 user 文本), 或元素列表(取 message.content 里
/// type=="input_text" 的 text; 图片/文件/音频类元素**跳过**——它们是素材不是提示词,
/// 混进来会让摘要变成一串路径, 反而看不出「这次问了什么」)。
///
/// 刻意不抛: 这是给人看的摘要, 形状不认识就少写一行, 不能让一次日志/列表渲染失败。
void collect_input_preview(const nlohmann::json& in_input, std::string& io_out) {
  if (in_input.is_string()) {
    io_out += in_input.get<std::string>() + "\n";
    return;
  }
  if (!in_input.is_array()) return;
  for (const auto& l_item : in_input) {
    if (!l_item.is_object()) continue;
    if (!l_item.contains("content")) continue;
    const auto& l_content = l_item.at("content");
    if (l_content.is_string()) {
      io_out += l_content.get<std::string>() + "\n";
      continue;
    }
    if (!l_content.is_array()) continue;
    for (const auto& l_part : l_content) {
      if (!l_part.is_object()) continue;
      if (!l_part.contains("type") || !l_part.at("type").is_string()) continue;
      if (l_part.at("type").get<std::string>() != "input_text") continue;
      if (l_part.contains("text") && l_part.at("text").is_string())
        io_out += l_part.at("text").get<std::string>() + "\n";
    }
  }
}
}  // namespace

ai_client_base::request_info_t seedance2_client::collect_request_info(const nlohmann::json& in_request) const {
  // 刻意保持宽容的字符串读键: 这个函数与**生成类**任务的 json 形状共用(前台提交路径),
  // 那种 json 不是归一化推理信封, 不能按推理信封的严格类型去解析 —— 何况那个结构体已经删掉了,
  // 现在只剩「直接读键」这一条路。这里只挑提示词用的字段读。
  request_info_t l_info{};
  if (in_request.contains("model")) l_info.model_ = in_request.at("model").get<std::string>();
  if (in_request.contains("resolution")) l_info.resolution_ = in_request.at("resolution").get<std::string>();
  // 推理信封(Responses)没有 resolution, 提示词在 input_text/instructions 里
  if (in_request.contains("instructions"))
    l_info.text_prompt_ += in_request.at("instructions").get<std::string>() + "\n";
  if (in_request.contains("input_text")) l_info.text_prompt_ += in_request.at("input_text").get<std::string>() + "\n";
  // 透传信封(**网关路径**)的提示词在 Responses 的 input 里, 不再是上面的扁平 input_text。
  // 不接这一支的话, 前台任务列表与日志上所有网关推理的提示词都是空的 —— 而「这次问了什么」
  // 恰恰是排查内容违规、比对计费时唯一要看的东西。
  //
  // 与上面两行同样是宽容读键: input 可以是字符串, 也可以是元素列表, 取不出来就跳过,
  // 绝不因为形状不是预期的而抛 —— 这只是给人看的摘要, 不是解析契约。
  if (in_request.contains("input")) collect_input_preview(in_request.at("input"), l_info.text_prompt_);
  // content 中 type 为 text 的项按顺序累加为提示词
  if (in_request.contains("content") && in_request.at("content").is_array())
    for (const auto& l_value : in_request.at("content"))
      if (l_value.contains("type") && l_value.at("type").get<std::string>() == "text" && l_value.contains("text"))
        l_info.text_prompt_ += l_value.at("text").get<std::string>() + "\n";
  return l_info;
}

boost::asio::awaitable<ai_client_base::run_task_result_t> seedance2_client::run_task(
    const doodle::seedance2::task& in_task
) {
  // 推理任务走同步的 Responses API: 一次调用就有结果, 不做异步提交
  if (in_task.type_ == doodle::seedance2::task_type::inference) co_return co_await run_inference_task(in_task);

  auto l_ip  = co_await get_ip_str();
  auto l_req = add_ip_to_req(
      in_task.data_request_, l_ip, url_sign_secret_,
      in_task.user_id_.is_nil() ? std::string{} : boost::uuids::to_string(in_task.user_id_)
  );

  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::post, std::string{g_generations_path}, 11
  };
  req.body() = l_req.dump();
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
  if (l_result.data_response_.contains("id") && l_res.result() == boost::beast::http::status::ok &&
      !l_result.data_response_.at("id").get<std::string>().empty())
    l_result.task_id_ = l_result.data_response_.at("id").get<std::string>();

  l_result.status_ =
      l_result.task_id_.empty() ? doodle::seedance2::task_status::failed : doodle::seedance2::task_status::queued;
  l_result.is_timeout_ = is_timeout_error(l_result.data_response_);
  l_result.error_code_ = get_error_code(l_result.data_response_);

  co_return l_result;
}

// ─── 推理 (Responses API) ────────────────────────────────────────────────────

// 请求体的定稿规则(model/store 覆盖)就地写在下面 run_inference_task() 里: 它是唯一的调用点,
// 于是不再单开一个头文件承载那个 inline 纯函数。

std::string seedance2_client::get_error_code(const nlohmann::json& in_body) {
  if (!in_body.contains("error") || !in_body.at("error").is_object()) return {};
  return in_body.at("error").value("code", std::string{});
}

bool seedance2_client::is_sensitive_content_error(const nlohmann::json& in_body) {
  const auto l_code = get_error_code(in_body);
  return l_code.find("SensitiveContent") != std::string::npos || l_code.find("PolicyViolation") != std::string::npos;
}

std::string seedance2_client::extract_output_text(const nlohmann::json& in_body) {
  if (!in_body.contains("output") || !in_body.at("output").is_array())
    return in_body.value("output_text", std::string{});

  std::string l_text{};
  for (const auto& l_item : in_body.at("output")) {
    if (!l_item.is_object()) continue;
    // 思考打开时 output[0] 是 reasoning 项, 只有 message 项才是答案
    if (l_item.value("type", std::string{}) != "message") continue;
    if (!l_item.contains("content") || !l_item.at("content").is_array()) continue;
    for (const auto& l_part : l_item.at("content")) {
      if (!l_part.is_object()) continue;
      if (l_part.value("type", std::string{}) != "output_text") continue;
      l_text += l_part.value("text", std::string{});
    }
  }
  return l_text;
}

std::string seedance2_client::strip_code_fence(std::string_view in_text) {
  const auto l_trim = [](std::string_view in_v) {
    const auto l_begin = in_v.find_first_not_of(" \t\r\n");
    if (l_begin == std::string_view::npos) return std::string_view{};
    return in_v.substr(l_begin, in_v.find_last_not_of(" \t\r\n") - l_begin + 1);
  };

  auto l_view = l_trim(in_text);
  if (!l_view.starts_with("```")) return std::string{l_view};

  const auto l_first_nl = l_view.find('\n');
  if (l_first_nl == std::string_view::npos) return std::string{l_view};
  l_view = l_view.substr(l_first_nl + 1);
  if (const auto l_close = l_view.rfind("```"); l_close != std::string_view::npos) l_view = l_view.substr(0, l_close);
  return std::string{l_trim(l_view)};
}

ai_client_base::run_task_result_t seedance2_client::parse_inference_response(
    const nlohmann::json& in_body, int in_status_code
) {
  run_task_result_t l_result{};
  l_result.data_response_ = in_body;
  l_result.error_code_    = get_error_code(in_body);
  l_result.is_timeout_    = is_timeout_error(in_body);

  if (in_body.contains("id") && in_body.at("id").is_string()) l_result.task_id_ = in_body.at("id").get<std::string>();

  const bool l_http_ok = in_status_code >= 200 && in_status_code < 300;
  if (l_http_ok && !in_body.contains("error")) {
    const auto l_status =
        in_body.contains("status") && in_body.at("status").is_string() ? in_body.at("status").get<std::string>() : "";
    // 没有 status 字段的兼容实现: 有 output 就当作成功
    l_result.status_ = (l_status == "completed" || (l_status.empty() && in_body.contains("output")))
                           ? doodle::seedance2::task_status::succeeded
                           : doodle::seedance2::task_status::failed;
  } else {
    l_result.status_ = doodle::seedance2::task_status::failed;
  }
  if (in_body.contains("usage") && in_body.at("usage").is_object()) {
    const auto& l_usage     = in_body.at("usage");
    l_result.input_tokens_  = l_usage.value("input_tokens", std::int64_t{0});
    l_result.output_tokens_ = l_usage.value("output_tokens", std::int64_t{0});
    l_result.total_tokens_  = l_usage.value("total_tokens", std::int64_t{0});
  }
  return l_result;
}

boost::asio::awaitable<ai_client_base::run_task_result_t> seedance2_client::run_inference_task(
    const doodle::seedance2::task& in_task
) {
  const auto l_uid = in_task.user_id_.is_nil() ? std::string{} : boost::uuids::to_string(in_task.user_id_);
  // 落库的 data_request_ **原样**打给方舟, 不经任何结构体往返一趟。
  //
  // 之前是 `data_request_.get<inference_request_t>()` 再 build_inference_body: 那次转换是
  // 一份**手写白名单**, 结构体不认识的键会被静默吃掉 —— 与「前端请求原样透传」直接冲突。
  // 那个结构体与那个函数现在都已删除, 这条纪律由**类型系统**保证: 中间已经没有可丢键的类型了。
  //
  // 现在这行只有两个动作: 定稿(剥本系统字段 / 覆盖强制字段 / 隐私口径), 然后发出去。
  // 接缝路径落库的也是同一份形状的信封, 所以两条路径在这里合流, 没有第二套发射规则。
  auto l_body      = in_task.data_request_;
  if (!l_body.is_object()) {
    // 非对象连 model 都没处放, 发出去必然被方舟拒且看不出原因, 不如当场说清楚是任务行坏了。
    throw_exception(
        http_request_error{
            boost::beast::http::status::internal_server_error,
            fmt::format(
                "推理请求体不是 JSON 对象(任务 {}): {}", boost::uuids::to_string(in_task.uuid_id_), l_body.type_name()
            )
        }
    );
  }
  // ---- 定稿: 覆盖服务端的强制字段(原地改) ----
  //
  // 这是 `/responses` 请求体与「本系统自己的请求」之间的边界: 过了这里的内容就只有方舟的
  // schema 在管, 服务端不再往里加任何字段 —— 所以它**只做覆盖**, 不替前端补默认值。
  //
  // 这段逻辑曾经是 seedance2_inference_request.h 里的一个 inline 自由函数
  // (finalize_inference_body), 库内只有下面这一个调用点, 其余调用者全是测试 ——
  // 为一个调用点维持一个头文件不合算, 就地写在这里。
  {
    // 传了空串（会话没配模型）时也要回落到缺省值, 否则会带着 model:"" 打过去白跑一趟;
    // 非空则**以调用方为准**（服务端不覆盖非空值, 与「原样透传」一致）。
    const auto l_model = l_body.contains("model") && l_body.at("model").is_string()
                             ? l_body.at("model").get<std::string>()
                             : std::string{};
    l_body["model"]    = l_model.empty() ? std::string{doodle_config::g_ark_inference_model} : l_model;
    // 不把提示词与素材留在方舟侧 —— 两条路径同一条隐私口径, 由这里统一保证。
    // store 是**唯一**一个服务端强行写入的键; 除此之外这里不补任何默认值（包括 thinking:
    // 以前这里发 {"type":"disabled"}, 那是服务端替前端做决定, 与「原样透传」冲突）。
    l_body["store"] = false;
    // max_output_tokens 同理**不补**：结构化输出要求全字段 required, 输出长度不可预估, 而截断
    // 之后 result_json_ 只剩半个 JSON —— 那个失败面跟「schema 不合规被拒」完全分不出来。
    // 交给模型/账号侧决定上限; 只有调用方/前端显式给了才发（它在体里, 原样留着, 这里不动）。
  }

  run_task_result_t l_result{};

  // 一次尝试, 不重试。曾经这里是一个 `for (l_attempt < g_ark_inference_max_retry)` 循环,
  // 按 should_retry(429/5xx/408) 判定是否再来一次并做指数退避。**整个重试已取消**:
  // 重试的每一步都要重新取 IP 并重新签名, 换来的却是同一份请求体被重复计费的风险 ——
  // 而调用方(前端 / 推理网关)自己就看得见 HTTP 状态码与 error_code, 需要重试时它自己重发
  // 更清楚上下文(比如换个提示词), 服务端替它盲重试反而把「哪一次真的产生了输出」这件事
  // 掩盖掉。取消后本函数的语义变得干净: **一次调用 = 一次请求 = 一次结算**。
  {
    // 每次尝试都重新取 IP 并**重新签名**
    auto l_ip       = co_await get_ip_str();
    auto l_req_json = add_ip_to_req(l_body, l_ip, url_sign_secret_, l_uid);

    boost::beast::http::request<boost::beast::http::string_body> req{
        boost::beast::http::verb::post, std::string{g_responses_path}, 11
    };
    req.body() = l_req_json.dump();
    req.set(boost::beast::http::field::content_type, "application/json");
    req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token_));
    req.set(boost::beast::http::field::accept, "application/json");
    req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_);
    req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
    http_client_ptr_->set_timeout(doodle_config::g_ark_inference_timeout);

    boost::beast::http::response<http::basic_json_body> l_res{};
    co_await http_client_ptr_->read_and_write(req, l_res, boost::asio::use_awaitable);
    const auto l_status_code = static_cast<int>(l_res.result_int());
    l_result                 = parse_inference_response(l_res.body(), l_status_code);
  }

  co_return l_result;
}

boost::asio::awaitable<ai_client_base::query_task_result_t> seedance2_client::query_task(
    const doodle::seedance2::task& in_task
) {
  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::get, fmt::format("/api/v3/contents/generations/tasks/{}", in_task.task_id_), 11
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
  l_result.status_        = parse_status(l_result.data_response_);
  l_result.is_timeout_    = is_timeout_error(l_result.data_response_);

  if (l_result.data_response_.contains("content") && l_result.data_response_.at("content").contains("video_url"))
    l_result.result_files_.push_back(l_result.data_response_.at("content").at("video_url").get<std::string>());
  if (l_result.data_response_.contains("usage") && l_result.data_response_.at("usage").contains("completion_tokens"))
    l_result.completion_tokens_ = l_result.data_response_.at("usage").at("completion_tokens").get<std::int64_t>();

  co_return l_result;
}

boost::asio::awaitable<void> seedance2_client::cancel_task(const std::string& in_task_id) {
  boost::beast::http::request<boost::beast::http::empty_body> req{
      boost::beast::http::verb::delete_, fmt::format("/api/v3/contents/generations/tasks/{}", in_task_id), 11
  };
  req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token_));
  req.set(boost::beast::http::field::accept, "application/json");
  req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_);
  req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
  boost::beast::http::response<boost::beast::http::empty_body> l_res{};
  co_await http_client_ptr_->read_and_write(req, l_res, boost::asio::use_awaitable);
  DOODLE_CHICK(l_res.result() == boost::beast::http::status::ok, "cancel_task error: {}", l_res.result());
}

boost::asio::awaitable<void> seedance2_client::download_result(query_task_result_t* in_data) {
  for (auto& l_url : in_data->result_files_) {
    auto l_raw_file = co_await download_raw_file(l_url);
    auto l_adj_file =
        l_raw_file.parent_path() / (l_raw_file.stem().string() + "_adj" + l_raw_file.extension().string());
    ffmpeg_video_resize{l_raw_file, l_adj_file, core_set::get_set().get_uuid()}.process();
    FSys::remove(l_raw_file);
    in_data->result_file_paths_.push_back(l_adj_file);
  }
}

std::size_t seedance2_client::default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const {
  switch (in_task_type) {
    case doodle::seedance2::task_type::video:
      return doodle_config::g_max_task_completion_tokens;
    case doodle::seedance2::task_type::inference:
      return doodle_config::g_ark_inference_default_consumed_tokens;
    default:
      return 5000;
  }
}

}  // namespace doodle::http::seedance2