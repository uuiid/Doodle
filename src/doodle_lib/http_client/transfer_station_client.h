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

  boost::asio::awaitable<run_task_result_t> run_task(const nlohmann::json& in_task) override;
  boost::asio::awaitable<query_task_result_t> query_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<void> cancel_task(const std::string& in_task_id) override;
  boost::asio::awaitable<void> download_result(query_task_result_t* in_data) override;

  request_info_t collect_request_info(const nlohmann::json& in_request) const override;
  std::size_t default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const override;

 private:
  static nlohmann::json add_ip_to_req(const nlohmann::json& in_req, std::string_view in_ip);
  static doodle::seedance2::task_status parse_status(const nlohmann::json& in_body);
};

}  // namespace doodle::http::seedance2