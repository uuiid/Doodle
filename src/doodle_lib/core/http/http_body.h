//
// Created by TD on 2025/10/26.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <boost/beast.hpp>

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace doodle::http {

struct http_body_base_t {
 private:
  boost::beast::http::status status_{boost::beast::http::status::ok};
  std::string_view mine_type_{"application/json; charset=utf-8"};
  bool has_cache_control_{false};
  bool is_deflate_{false};
  bool is_attachment_{false};
  std::string attachment_filename_{};

 protected:
  http_body_base_t() = default;
  http_body_base_t(boost::beast::http::status in_status, std::string_view in_mine_type);

  http_body_base_t& set_status(boost::beast::http::status in_status);
  http_body_base_t& set_mine_type(std::string_view in_mine_type);
  http_body_base_t& set_deflate(bool in_deflate = true);
  http_body_base_t& set_cache_control(bool in_enable = true);
  http_body_base_t& set_attachment(std::string in_filename = {});

 public:
  virtual ~http_body_base_t() = default;

  [[nodiscard]] boost::beast::http::status get_status() const;
  [[nodiscard]] std::string_view get_mine_type() const;
  [[nodiscard]] bool has_cache_control() const;
  [[nodiscard]] bool is_deflate() const;
  [[nodiscard]] bool is_attachment() const;
  [[nodiscard]] const std::string& get_attachment_filename() const;
};

using http_body_ptr = std::unique_ptr<http_body_base_t>;

struct http_body_json_t : http_body_base_t {
  nlohmann::json body_{};

  http_body_json_t() = default;
  explicit http_body_json_t(
      nlohmann::json in_body, boost::beast::http::status in_status = boost::beast::http::status::ok
  );
};

struct http_body_string_t : http_body_base_t {
  std::string body_{};

  http_body_string_t() = default;
  explicit http_body_string_t(
      std::string in_body, std::string_view in_mime_type = "text/plain; charset=utf-8",
      boost::beast::http::status in_status = boost::beast::http::status::ok
  );
};

struct http_body_binary_t : http_body_base_t {
  std::vector<std::byte> body_{};

  http_body_binary_t() = default;
  explicit http_body_binary_t(
      std::vector<std::byte> in_body, std::string_view in_mime_type = "application/octet-stream",
      boost::beast::http::status in_status = boost::beast::http::status::ok
  );
  explicit http_body_binary_t(
      std::vector<unsigned char> in_body, std::string_view in_mime_type = "application/octet-stream",
      boost::beast::http::status in_status = boost::beast::http::status::ok
  );
};

struct http_body_file_t : http_body_base_t {
  FSys::path path_{};

  http_body_file_t() = default;
  explicit http_body_file_t(
      FSys::path in_path, std::string_view in_mime_type,
      boost::beast::http::status in_status = boost::beast::http::status::ok
  );
};

struct http_body_empty_t : http_body_base_t {
  http_body_empty_t();
};

struct http_body_error_t : http_body_base_t {
  std::string message_{};
  std::int32_t code_{-1};

  explicit http_body_error_t(
      boost::beast::http::status in_status, std::string in_message, std::int32_t in_code = -1
  );
};

}  // namespace doodle::http
