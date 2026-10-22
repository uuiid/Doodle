//
// Created by TD on 2024/2/29.
//

#pragma once

#include <doodle_core/metadata/seedance2/task.h>

#include <doodle_lib/core/http_client_core.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <chrono>
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

  void set_logger(logger_ptr in_logger) { logger_ = in_logger ? std::move(in_logger) : spdlog::default_logger(); }
  void set_token(const std::string& in_token) { token_ = in_token; }
  const std::string& get_token() const { return token_; }

  void set_url_sign_secret(const std::string& in_secret) { url_sign_secret_ = in_secret; }
  const std::string& get_url_sign_secret() const { return url_sign_secret_; }

  struct run_task_result_t {
    std::string task_id_;
    doodle::seedance2::task_status status_;
    bool is_timeout_{false};
    nlohmann::json data_response_;

    std::string result_text_{};
    nlohmann::json result_json_{};
    std::vector<std::string> result_files_;
    std::vector<FSys::path> result_file_paths_;
    std::int64_t input_tokens_{0};
    std::int64_t output_tokens_{0};
    std::int64_t total_tokens_{0};
    std::int64_t completion_tokens_{0};
  };

  struct request_info_t {
    std::string model_{};
    std::string resolution_{};
    std::string text_prompt_{};
  };

  virtual boost::asio::awaitable<run_task_result_t> submit_task(const doodle::seedance2::task& in_task) = 0;

  virtual boost::asio::awaitable<run_task_result_t> run_task(const doodle::seedance2::task& in_task);

  virtual boost::asio::awaitable<void> cancel_task(const std::string& in_task_id)                      = 0;

  virtual boost::asio::awaitable<run_task_result_t> query_task(const doodle::seedance2::task& in_task) = 0;

  virtual boost::asio::awaitable<void> download_result(run_task_result_t& in_data)                     = 0;

  virtual request_info_t collect_request_info(const nlohmann::json& in_request) const                  = 0;
  virtual std::size_t default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const  = 0;

  static std::string to_public_url(
      std::string_view in_url, std::string_view in_ip, std::string_view in_secret = {}, std::string_view in_uid = {}
  );

  static nlohmann::json add_ip_to_req(
      const nlohmann::json& in_req, std::string_view in_ip, std::string_view in_secret = {},
      std::string_view in_uid = {}
  );

  template <typename Fn>
  static void for_each_media_url_in_content(nlohmann::json& in_content, Fn&& in_fn) {
    if (!in_content.is_array()) return;
    for (auto& l_item : in_content) {
      if (!l_item.is_object()) continue;
      for (const auto* l_key : {"image_url", "video_url", "audio_url"}) {
        if (!l_item.contains(l_key)) continue;
        auto& l_holder = l_item.at(l_key);
        if (l_holder.is_object() && l_holder.contains("url") && l_holder.at("url").is_string())
          in_fn(l_holder.at("url"));
        else if (l_holder.is_string())
          in_fn(l_holder);
      }
    }
  }

  template <typename Fn>
  static void for_each_media_url(nlohmann::json& in_req, Fn&& in_fn) {
    if (!in_req.is_object()) return;
    if (in_req.contains("content")) for_each_media_url_in_content(in_req.at("content"), in_fn);
    if (in_req.contains("input") && in_req.at("input").is_array())
      for (auto& l_input : in_req.at("input"))
        if (l_input.is_object() && l_input.contains("content"))
          for_each_media_url_in_content(l_input.at("content"), in_fn);
  }

 protected:
  boost::asio::awaitable<std::string> get_ip_str();
  boost::asio::awaitable<FSys::path> download_raw_file(std::string_view in_file_url);

  std::string url_sign_secret_{};

  std::string cached_ip_{};
  std::chrono::steady_clock::time_point cached_ip_expire_{};
};
}  // namespace doodle::http::seedance2
