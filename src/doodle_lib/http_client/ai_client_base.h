//
// Created by TD on 2024/2/29.
//

#pragma once

#include <doodle_core/metadata/seedance2/task.h>

#include <doodle_lib/core/http_client_core.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

namespace doodle::http::seedance2 {
class DOODLELIB_API ai_client_base : public std::enable_shared_from_this<ai_client_base> {
  using http_client_t     = doodle::http::http_client_ssl;
  using http_client_ptr_t = std::shared_ptr<http_client_t>;

 protected:
  http_client_ptr_t http_client_ptr_{};
  std::string token_;
  logger_ptr logger_{spdlog::default_logger()};

 public:
  explicit ai_client_base(std::string in_host_url, boost::asio::ssl::context& in_ctx)
      : http_client_ptr_{std::make_shared<http_client_t>(std::move(in_host_url), in_ctx)} {}

  void set_logger(logger_ptr in_logger) { logger_ = std::move(in_logger); }
  void set_token(const std::string& in_token) { token_ = in_token; }
  const std::string& get_token() const { return token_; }

  struct run_task_result_t {
    std::string task_id_;
    doodle::seedance2::task_status status_;
    bool is_timeout_{false};
    nlohmann::json data_response_;
  };

  struct query_task_result_t {
    std::shared_ptr<ai_client_base> client_ptr_;

    doodle::seedance2::task_status status_;
    bool is_timeout_{false};
    std::int64_t completion_tokens_{0};
    nlohmann::json data_response_;
    std::vector<std::string> result_files_;
    std::vector<FSys::path> result_file_paths_;
    boost::asio::awaitable<void> download();
  };

  virtual boost::asio::awaitable<run_task_result_t> run_task(const nlohmann::json& in_task)     = 0;
  virtual boost::asio::awaitable<void> cancel_task(const std::string& in_task_id)               = 0;
  virtual boost::asio::awaitable<query_task_result_t> query_task(const std::string& in_task_id) = 0;
  virtual boost::asio::awaitable<void> download_result(query_task_result_t* in_data)            = 0;

 protected:
  boost::asio::awaitable<std::string> get_ip_str();
};
}  // namespace doodle::http::seedance2