#pragma once

#include <doodle_core/metadata/seedance2/image_request.h>
#include <doodle_core/metadata/seedance2/task.h>

#include "ai_client_base.h"
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace doodle::http::seedance2 {

class DOODLELIB_API apipy_image_client final : public ai_client_base {
  constexpr static std::string_view g_default_host{"https://api.apiyi.com"};

 public:
  constexpr static std::chrono::seconds g_request_timeout{360};
  constexpr static std::string_view g_default_model{"gpt-image-2.5-vip"};

  constexpr static std::string_view g_edits_path{"/v1/images/edits"};

  constexpr static std::string_view g_edit_image_field{"image"};

  constexpr static std::size_t g_max_reference_images{8};

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

  boost::asio::awaitable<run_task_result_t> submit_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<run_task_result_t> query_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<void> cancel_task(const std::string& in_task_id) override;
  boost::asio::awaitable<void> download_result(run_task_result_t& in_data) override;

  request_info_t collect_request_info(const nlohmann::json& in_request) const override;
  std::size_t default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const override;

  static std::vector<std::string> extract_image_urls(const nlohmann::json& in_body);

  static std::string extract_b64_payload(const nlohmann::json& in_body);

  static doodle::seedance2::task_status parse_status(const nlohmann::json& in_body, int in_status_code);

  static std::string get_error_code(const nlohmann::json& in_body);

  [[nodiscard]] static nlohmann::json normalize_apipy_request(const nlohmann::json& in_request);

  static edits_form_t build_edit_multipart(
      const nlohmann::json& in_apipy_body, const std::vector<edit_reference_t>& in_references
  );

  static FSys::path parse_site_image_id(std::string_view in_url);

  static std::string mime_of_image(const std::string& in_ext);

 private:
  FSys::path write_b64_to_cache(std::string_view in_b64, std::string_view in_ext) const;

  apipy_image_client::edit_reference_t load_reference_image(std::string_view in_url);
};

}  // namespace doodle::http::seedance2
