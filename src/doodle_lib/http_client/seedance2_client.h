//
// Created by TD on 2024/2/29.
//

#pragma once

#include <doodle_core/metadata/seedance2/task.h>

#include "ai_client_base.h"
#include "seedance2_inference_request.h"
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>

namespace doodle::http::seedance2 {

/**
 * 火山方舟(volcengine ark) 客户端。
 *
 * 两套接口:
 * - 生成: POST /api/v3/contents/generations/tasks  (异步, 提交后靠 query_task 轮询)
 * - 推理: POST /api/v3/responses                    (同步, 一次调用拿结果)
 *
 * 推理请求体由调用方按**归一化信封**给出(data_request_), 由 build_inference_body() 翻成
 * 方舟格式; 归一化信封的字段见 inference_request_t(seedance2_inference_request.h)。
 */
class DOODLELIB_API seedance2_client final : public ai_client_base {
  constexpr static std::string_view g_sd2_host_url{"https://ark.cn-beijing.volces.com"};

 public:
  explicit seedance2_client(boost::asio::ssl::context& in_ctx) : ai_client_base(std::string{g_sd2_host_url}, in_ctx) {}

  boost::asio::awaitable<run_task_result_t> run_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<query_task_result_t> query_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<void> cancel_task(const std::string& in_task_id) override;
  boost::asio::awaitable<void> download_result(query_task_result_t* in_data) override;

  request_info_t collect_request_info(const nlohmann::json& in_request) const override;

  static bool is_timeout_error(const nlohmann::json& in_body);
  std::size_t default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const override;

  // ─── 推理(Responses API) ───────────────────────────────────────────────────
  // 下面几个都是纯函数, 便于不联网单测。

  /**
   * 把归一化请求信封(inference_request_t)翻成方舟 Responses 请求体。
   * 字段语义与缺省值见 inference_request_t 的注释; 固定 store=false: 不把提示词/素材留在方舟侧。
   * 传进来的信封已是强类型, 这里不再做任何字符串读键/类型猜测。
   */
  static nlohmann::json build_inference_body(const inference_request_t& in_request);

  /// 解析 Responses 响应; in_status_code 是 HTTP 状态码
  static run_task_result_t parse_inference_response(const nlohmann::json& in_body, int in_status_code);

  /// 抽取 output[] 中 type=="message" 的 output_text 拼接结果。
  /// 注意 thinking 打开时 output[0] 是 reasoning 项, 不能直接取 output[0]。
  static std::string extract_output_text(const nlohmann::json& in_body);

  /// 取 error.code(没有则空串)
  static std::string get_error_code(const nlohmann::json& in_body);

  /// 是否是内容违规类错误(400 SensitiveContentDetected* / *PolicyViolation)
  static bool is_sensitive_content_error(const nlohmann::json& in_body);

  /// 是否值得重试: 429 / 5xx / 408
  static bool should_retry(int in_status_code, std::string_view in_error_code);

  /// 去掉 ```json ... ``` 代码围栏(只用于解析 JSON, 落库的文本保留原文)
  static std::string strip_code_fence(std::string_view in_text);

 private:
  static doodle::seedance2::task_status parse_status(const nlohmann::json& in_body);

  /// 推理任务: 一次调用(内部按需重试)拿到结果
  boost::asio::awaitable<run_task_result_t> run_inference_task(const doodle::seedance2::task& in_task);
};

}  // namespace doodle::http::seedance2
