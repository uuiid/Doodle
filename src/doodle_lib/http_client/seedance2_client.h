//
// Created by TD on 2024/2/29.
//

#pragma once

#include <doodle_core/metadata/seedance2/task.h>

#include <doodle_lib/core/http_client_core.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <nlohmann/json_fwd.hpp>

namespace doodle::http::seedance2 {

class seedance2_client : public std::enable_shared_from_this<seedance2_client> {
  using http_client_t     = doodle::http::http_client_ssl;
  using http_client_ptr_t = std::shared_ptr<http_client_t>;

  http_client_ptr_t http_client_ptr_{};
  std::string token_;
  logger_ptr logger_{spdlog::default_logger()};

 public:
  explicit seedance2_client(boost::asio::ssl::context& in_ctx)
      : http_client_ptr_{std::make_shared<http_client_t>("https://ark.cn-beijing.volces.com", in_ctx)} {}

  void set_logger(logger_ptr in_logger) { logger_ = std::move(in_logger); }
  void set_token(const std::string& in_token) { token_ = in_token; }
  const std::string& get_token() const { return token_; }

  boost::asio::awaitable<std::string> run_task(const nlohmann::json& in_task);

  // 查询任务
  boost::asio::awaitable<nlohmann::json> query_task(const std::string& in_task_id);
};
}  // namespace doodle::http::seedance2