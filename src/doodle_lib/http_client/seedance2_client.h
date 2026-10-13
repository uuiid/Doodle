//
// Created by TD on 2024/2/29.
//

#pragma once

#include <doodle_core/metadata/seedance2/task.h>

#include "ai_client_base.h"
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
 * 两者都实现在 submit_task 里(由它按 in_task.type_ 分派), run_task 沿用基类默认转发。
 *
 * 推理请求体由调用方按**扁平信封**给出(data_request_), 在 run_inference_task 里就地把
 * model/store 定稿后原样发给方舟 —— 中间不经过任何结构体往返(那个 inference_request_t
 * 已删除), 定稿逻辑也只有那一个调用点, 因此直接写在调用处, 不再单开一个头文件。
 */
class DOODLELIB_API seedance2_client final : public ai_client_base {
  constexpr static std::string_view g_sd2_host_url{"https://ark.cn-beijing.volces.com"};

 public:
  explicit seedance2_client(boost::asio::ssl::context& in_ctx) : ai_client_base(std::string{g_sd2_host_url}, in_ctx) {}

  /// 提交任务。生成类走异步生成接口(返回 queued), 推理类走 Responses API(一次调用即终态)。
  boost::asio::awaitable<run_task_result_t> submit_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<query_task_result_t> query_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<void> cancel_task(const std::string& in_task_id) override;
  boost::asio::awaitable<void> download_result(query_task_result_t* in_data) override;

  request_info_t collect_request_info(const nlohmann::json& in_request) const override;

  static bool is_timeout_error(const nlohmann::json& in_body);
  std::size_t default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const override;

  // ─── 推理(Responses API) ───────────────────────────────────────────────────
  // 下面几个都是纯函数, 便于不联网单测。

  /// 解析 Responses 响应; in_status_code 是 HTTP 状态码
  static run_task_result_t parse_inference_response(const nlohmann::json& in_body, int in_status_code);

  /// 抽取 output[] 中 type=="message" 的 output_text 拼接结果。
  /// 注意 thinking 打开时 output[0] 是 reasoning 项, 不能直接取 output[0]。
  static std::string extract_output_text(const nlohmann::json& in_body);

  /// 取 error.code(没有则空串)
  static std::string get_error_code(const nlohmann::json& in_body);

  /// 是否是内容违规类错误(400 SensitiveContentDetected* / *PolicyViolation)
  static bool is_sensitive_content_error(const nlohmann::json& in_body);

  /// 去掉 ```json ... ``` 代码围栏(只用于解析 JSON, 落库的文本保留原文)
  static std::string strip_code_fence(std::string_view in_text);

 private:
  static doodle::seedance2::task_status parse_status(const nlohmann::json& in_body);

  /// 推理任务: 一次调用**只发一次请求**(自动重试已取消, 见 .cpp 里的说明)
  boost::asio::awaitable<run_task_result_t> run_inference_task(const doodle::seedance2::task& in_task);
};

}  // namespace doodle::http::seedance2
