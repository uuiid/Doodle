#pragma once

#include <doodle_core/metadata/seedance2/image_request.h>
#include <doodle_core/metadata/seedance2/task.h>

#include "ai_client_base.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace doodle::http::seedance2 {

/// Google 官方 Gemini(「Nano Banana」系列) 图片生成后端。
///
/// 与 API易 后端一样是**同步**后端: 一次 POST 就是终态, 没有 task_id 可查,
/// 所以 submit_task 里直接拿到图片字节, 失败即终态失败 —— 调用方(seedance2_task_run_manager)
/// 看不到中间的 running/queued, 也就不会重复提交。
///
/// 与 API易 的**区别**是响应形状: Gemini 不返回 URL, 图片永远以 base64 内联在响应里,
/// 所以这里没有「先取 url 再下载」那条分支, 一律 base64 解码后落盘。
class DOODLELIB_API gemini_image_client final : public ai_client_base {
  constexpr static std::string_view g_default_host{"https://generativelanguage.googleapis.com"};

 public:
  /// 一帧参考图: 本站图片路由映射到本地文件后读出来的字节。
  struct reference_image_t {
    std::string url_;
    std::string mime_;
    std::string bytes_;
  };

  /// interactions 响应里的一帧结果图。
  struct output_image_t {
    std::string data_;
    std::string mime_type_;
  };

  struct response_t {
    std::string interaction_id_;
    std::string status_;
    std::string text_;
    std::vector<output_image_t> images_;
    std::string error_code_;
    std::string error_message_;
    std::int64_t input_tokens_{0};
    std::int64_t output_tokens_{0};
    std::int64_t total_tokens_{0};
    bool has_error_{false};

    [[nodiscard]] bool has_images() const { return !images_.empty(); }

    [[nodiscard]] doodle::seedance2::task_status get_status(int in_status_code) const;
  };

  explicit gemini_image_client(boost::asio::ssl::context& in_ctx)
      : ai_client_base(std::string{g_default_host}, in_ctx) {}

  struct request_t {
    std::string model_;
    std::string prompt_;
    std::string aspect_ratio_;
    std::string image_size_;
    std::vector<reference_image_t> references_;

    explicit request_t(const doodle::seedance2::transfer_station_request_t& in_req);

    [[nodiscard]] nlohmann::json to_body() const;
  };

  friend void to_json(nlohmann::json& j, const request_t& req);

  boost::asio::awaitable<run_task_result_t> submit_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<run_task_result_t> run_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<run_task_result_t> query_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<void> cancel_task(const std::string& in_task_id) override;
  boost::asio::awaitable<void> download_result(run_task_result_t& in_data) override;

  request_info_t collect_request_info(const nlohmann::json& in_request) const override;
  std::size_t default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const override;

  [[nodiscard]] static response_t parse_response(const nlohmann::json& in_body);

  /// 把一次响应收缩成可入库的形态: 剥掉 base64 图片与内联图片输入, 只留下 id/status/usage 与文字。
  [[nodiscard]] static nlohmann::json strip_image_payload(nlohmann::json in_body);

  /// 把中转站的 aspectRatio / imageSize 对齐到 Gemini 的取值:
  /// aspectRatio 只认 "16:9" 这种; imageSize 只认 1K / 2K / 4K (大写 K, 小写会被上游拒绝)。
  [[nodiscard]] static std::string normalize_aspect_ratio(std::string_view in_ratio);
  [[nodiscard]] static std::string normalize_image_size(std::string_view in_size);

  static FSys::path parse_site_image_id(std::string_view in_url);
  static std::string mime_of_image(const std::string& in_ext);
  static std::string ext_of_mime(std::string_view in_mime);

 private:
  FSys::path write_b64_to_cache(std::string_view in_b64, std::string_view in_ext) const;

  reference_image_t load_reference_image(std::string_view in_url);

  [[nodiscard]] std::shared_ptr<doodle::http::http_client_ssl> make_own_client() const;
};

}  // namespace doodle::http::seedance2
