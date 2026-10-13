//
// Created by TD on 2025.
//

#pragma once

#include <doodle_core/metadata/seedance2/task.h>

#include "ai_client_base.h"
#include <string>
#include <string_view>

namespace doodle::http::seedance2 {

class DOODLELIB_API transfer_station_client final : public ai_client_base {
  constexpr static std::string_view g_default_host{"https://grsai.dakka.com.cn"};

 public:
  explicit transfer_station_client(boost::asio::ssl::context& in_ctx)
      : ai_client_base(std::string{g_default_host}, in_ctx) {}

  /// 提交任务: 走中转站的异步生成接口, 返回 queued, 之后靠 query_task 轮询。
  boost::asio::awaitable<run_task_result_t> submit_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<query_task_result_t> query_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<void> cancel_task(const std::string& in_task_id) override;
  boost::asio::awaitable<void> download_result(query_task_result_t* in_data) override;

  request_info_t collect_request_info(const nlohmann::json& in_request) const override;
  std::size_t default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const override;

 private:
  /// 中转站自己的请求形状: images[] 是「字符串数组」, mask 是「字符串」,
  /// 与生成 API 的 content[].image_url.url 不同, 所以单独遍历;
  /// 但单个 URL 的改写规则与其它后端一致(见 ai_client_base::to_public_url)。
  static nlohmann::json add_ip_to_req(
      const nlohmann::json& in_req, std::string_view in_ip, std::string_view in_secret = {},
      std::string_view in_uid = {}
  );
  static doodle::seedance2::task_status parse_status(const nlohmann::json& in_body);
};

}  // namespace doodle::http::seedance2