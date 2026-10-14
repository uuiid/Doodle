//
// Created by TD on 2025.
//

#pragma once

#include <doodle_core/metadata/seedance2/task.h>

#include "ai_client_base.h"
#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>

namespace doodle::http::seedance2 {

/**
 * API易(apiyi.com) 图片客户端。
 *
 * 与另外两个子类**根本不同**: 这个后端的图片接口是**同步单次调用** ——
 * 一次 HTTP 请求全程阻塞, 结果直接在响应里返回。
 *   * 没有 task_id, 没有查询接口, 没有取消接口;
 *   * 客户端断开后上游仍会跑完并**照常计费**(官方最佳实践页明确写了这一点)。
 * 所以 timeout 是本后端的第一原则, 见 g_request_timeout。
 *
 * 于是基类的流程纯虚在这里只能是:
 *   * submit_task  —— 真正干活的那个: 一次调用直接拿终态 + 图片文件;
 *   * query_task   —— 没有对应接口, 永远不该被调用(见 .cpp 的说明);
 *   * cancel_task  —— 同上;
 *   * download_result —— 下载动作发生在 submit_task 内部, 所以也没有调用者。
 *
 * 请求形状(data_request_)沿用工程内既有的「扁平信封」, 即前端直接把该模型的请求体
 * 塞进 data_request_, 客户端只做定稿与素材 URL 改写, 不做结构体往返:
 *   { "model": "gpt-image-2.5-vip", "prompt": "...", "size": "2048x1152", ... }
 */
class DOODLELIB_API apipy_image_client final : public ai_client_base {
  constexpr static std::string_view g_default_host{"https://api.apiyi.com"};

 public:
  /// 本后端是同步长请求, 一次调用阻塞到出图。
  /// 官方最佳实践给的推荐值是 GPT-Image-2.5 系列 360 秒(GPT-Image-2-vip 是 300 秒),
  /// 这里取 360 起步。为什么必须留足: 客户端主动断开后上游仍会跑完并**照常计费**
  /// (官方原话), 所以 timeout 设小了不是"快速失败", 而是"花了钱拿不到图"。
  /// 注意这不是本链路唯一的上限 —— http_client_core.h 的 read_and_write 内部还有一次
  /// 硬编码的 expires_after(900s) 覆盖「写请求 + 读响应头」, 两者取小, 因此真正的天花板是
  /// 900 秒。将来本值若调到 900 以上, 必须同步改那一处。
  ///
  /// 放在这里而不是 doodle_core/configure/static_value.h: 那个头是全工程范围的,
  /// 动它会触发全量重编(实测 469 个编译单元), 而这两个常量只有本文件用。
  constexpr static std::chrono::seconds g_request_timeout{360};
  /// 默认模型; 请求体里带了非空 model 就用请求体的
  constexpr static std::string_view g_default_model{"gpt-image-2.5-vip"};

  explicit apipy_image_client(boost::asio::ssl::context& in_ctx)
      : ai_client_base(std::string{g_default_host}, in_ctx) {}

  /// 一次调用即终态: 发请求 → 解析 → **当场把图片下载落盘** → 填 result_files_ /
  /// result_file_paths_ 返回。本后端没有 query_task 可以之后再取, 也没必要重写 run_task
  /// (基类默认就是转发到这里)。
  boost::asio::awaitable<run_task_result_t> submit_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<run_task_result_t> query_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<void> cancel_task(const std::string& in_task_id) override;
  boost::asio::awaitable<void> download_result(run_task_result_t& in_data) override;

  request_info_t collect_request_info(const nlohmann::json& in_request) const override;
  std::size_t default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const override;

  // ─── 纯函数, 便于不联网单测 ────────────────────────────────────────────────

  /// 从响应体里取出图片 URL 列表。
  /// 兼容三种形状: data[0].url(R2 链接) / data[0].b64_json(历史版本含 data: 前缀) /
  /// 以及上游报错时的 error 对象(返回空)。
  static std::vector<std::string> extract_image_urls(const nlohmann::json& in_body);

  /// 取 data[0].b64_json 并剥掉可能存在的 data: 前缀(2026-07 实测为纯 base64,
  /// 但历史版本曾带前缀, 官方也要求做这个检测)。没有该字段时返回空串。
  static std::string extract_b64_payload(const nlohmann::json& in_body);

  /// 按 HTTP 状态码与响应体判定任务状态。
  /// 2xx 且有图 → succeeded; 其余 → failed(附错误码)。
  static doodle::seedance2::task_status parse_status(const nlohmann::json& in_body, int in_status_code);

  /// 取上游错误码(error.code / error.type); 没有则空串
  static std::string get_error_code(const nlohmann::json& in_body);

 private:
  /// base64 结果就地落盘(core_set 缓存目录), 返回文件路径。
  /// b64_json 模式(未切 image2_OSS 分组时会发生)走这条; url 模式走基类 download_raw_file。
  FSys::path write_b64_to_cache(std::string_view in_b64, std::string_view in_ext) const;
};

}  // namespace doodle::http::seedance2
