#pragma once

#include <doodle_core/metadata/seedance2/image_request.h>
#include <doodle_core/metadata/seedance2/task.h>

#include "ai_client_base.h"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace doodle::http {

class DOODLELIB_API kmt_image_client final : public ai_client_base {
  constexpr static std::string_view g_default_host{"https://api.52pay.com"};

 public:
  struct reference_image_t {
    std::string filename_;
    std::string bytes_;
    std::int32_t cols_{};
    std::int32_t rows_{};
  };

  struct edits_form_t {
    std::string boundary_;
    std::string body_;

    [[nodiscard]] std::string content_type() const { return "multipart/form-data; boundary=" + boundary_; }
  };

  struct request_t {
    std::string model_;
    std::string prompt_;
    std::string size_;
    std::string quality_;
    std::vector<reference_image_t> reference_images_;

    explicit request_t(const doodle::seedance2::transfer_station_request_t& in_req);

    [[nodiscard]] edits_form_t to_multipart() const;

    friend void to_json(nlohmann::json& j, const request_t& in_req);
  };

  struct data_item_t {
    std::string url_;
  };

  struct response_t {
    std::string id_;
    std::string object_;
    doodle::seedance2::task_status status_{doodle::seedance2::task_status::failed};
    std::string model_;
    std::int64_t created_{0};
    std::vector<data_item_t> data_;
  };

  friend void from_json(const nlohmann::json& j, data_item_t& in) { in.url_ = j.value("url", std::string{}); }
  friend void from_json(const nlohmann::json& j, response_t& in);

  explicit kmt_image_client(boost::asio::ssl::context& in_ctx) : ai_client_base(std::string{g_default_host}, in_ctx) {}

  boost::asio::awaitable<run_task_result_t> submit_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<run_task_result_t> query_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<void> cancel_task(const std::string& in_task_id) override;
  boost::asio::awaitable<void> download_result(run_task_result_t& in_data) override;

  request_info_t collect_request_info(const nlohmann::json& in_request) const override;
  std::size_t default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const override;

 private:
  static doodle::seedance2::task_status parse_status(std::string_view in_status);
};

}  // namespace doodle::http
