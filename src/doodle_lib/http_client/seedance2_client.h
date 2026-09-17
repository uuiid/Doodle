//
// Created by TD on 2024/2/29.
//

#pragma once

#include "ai_client_base.h"

#include <doodle_core/metadata/seedance2/task.h>

#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>

namespace doodle::http::seedance2 {

class DOODLELIB_API seedance2_client final : public ai_client_base {
  constexpr static std::string_view g_sd2_host_url{"https://ark.cn-beijing.volces.com"};

 public:
  explicit seedance2_client(boost::asio::ssl::context& in_ctx)
      : ai_client_base(std::string{g_sd2_host_url}, in_ctx) {}

  boost::asio::awaitable<run_task_result_t> run_task(const nlohmann::json& in_task) override;
  boost::asio::awaitable<query_task_result_t> query_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<void> cancel_task(const std::string& in_task_id) override;
  boost::asio::awaitable<void> download_result(query_task_result_t* in_data) override;

  request_info_t collect_request_info(const nlohmann::json& in_request) const override;

  static bool is_timeout_error(const nlohmann::json& in_body);

 private:
  static nlohmann::json add_ip_to_req(const nlohmann::json& in_req, std::string_view in_ip);
  static doodle::seedance2::task_status parse_status(const nlohmann::json& in_body);
};

}  // namespace doodle::http::seedance2