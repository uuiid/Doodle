#pragma once

#include <doodle_core/metadata/seedance2/image_request.h>
#include <doodle_core/metadata/seedance2/task.h>

#include "ai_client_base.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace doodle::http::seedance2 {

class DOODLELIB_API apipy_image_client final : public ai_client_base {
  constexpr static std::string_view g_default_host{"https://api.apiyi.com"};

 public:
  struct edits_form_t {
    std::string boundary_;
    std::string body_;

    [[nodiscard]] std::string content_type() const { return "multipart/form-data; boundary=" + boundary_; }
  };

  struct edit_reference_t {
    std::string url_;
    std::string filename_;
    std::string mime_;
    std::string bytes_;
  };

  struct response_t {
    std::string url_;
    std::string b64_json_;
    std::string error_code_;
    std::string error_message_;
    std::int64_t input_tokens_{0};
    std::int64_t output_tokens_{0};
    std::int64_t total_tokens_{0};
    bool has_error_{false};

    [[nodiscard]] bool has_url() const { return !url_.empty(); }
    [[nodiscard]] bool has_b64_json() const { return !b64_json_.empty(); }

    [[nodiscard]] doodle::seedance2::task_status get_status(int in_status_code) const;
  };

  explicit apipy_image_client(boost::asio::ssl::context& in_ctx)
      : ai_client_base(std::string{g_default_host}, in_ctx) {}

  struct request_t {
    std::string model_;
    std::string prompt_;
    std::string size_;
    std::string quality_;
    std::vector<edit_reference_t> image_bytes_;

    explicit request_t(const doodle::seedance2::transfer_station_request_t& in_req)
        : model_(in_req.model_),
          prompt_(in_req.prompt_),
          size_(in_req.image_size_.empty() ? in_req.aspect_ratio_ : in_req.image_size_),
          quality_(in_req.quality_) {}

    [[nodiscard]] edits_form_t to_multipart() const;
  };

  friend void to_json(nlohmann::json& j, const request_t& req);

  boost::asio::awaitable<run_task_result_t> run_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<run_task_result_t> query_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<void> cancel_task(const std::string& in_task_id) override;
  boost::asio::awaitable<void> download_result(run_task_result_t& in_data) override;

  request_info_t collect_request_info(const nlohmann::json& in_request) const override;
  std::size_t default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const override;

  [[nodiscard]] static response_t parse_response(const nlohmann::json& in_body);

  static FSys::path parse_site_image_id(std::string_view in_url);

  static std::string mime_of_image(const std::string& in_ext);

 private:
  FSys::path write_b64_to_cache(std::string_view in_b64, std::string_view in_ext) const;

  apipy_image_client::edit_reference_t load_reference_image(std::string_view in_url);
};

}  // namespace doodle::http::seedance2
