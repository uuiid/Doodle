//
// Created by TD on 2025/10/26.
//
#pragma once

#include <doodle_core/configure/static_value.h>
#include <doodle_core/doodle_core_fwd.h>

#include <boost/beast.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace doodle::http {

namespace detail {
class session_data;
}

enum class http_file_range_mode {
  full,
  head_only,
  partial,
};

struct http_body_base_t {
 private:
  boost::beast::http::status status_{boost::beast::http::status::ok};
  std::string_view mine_type_{"application/json; charset=utf-8"};
  bool has_cache_control_{false};
  bool is_deflate_{false};
  bool is_attachment_{false};
  std::string attachment_filename_{};
  std::vector<std::pair<boost::beast::http::field, std::string>> headers_{};

  static std::string generate_etag(const FSys::path& in_path);

 protected:
  http_body_base_t() = default;
  http_body_base_t(boost::beast::http::status in_status, std::string_view in_mine_type);

  http_body_base_t(const http_body_base_t&)            = delete;
  http_body_base_t& operator=(const http_body_base_t&) = delete;
  http_body_base_t(http_body_base_t&&)                 = default;
  http_body_base_t& operator=(http_body_base_t&&)      = default;

  boost::beast::http::status body_status(const detail::session_data& in_session_data) const;

 public:
  virtual ~http_body_base_t() = default;

  template <typename T>
  void set_response_header(
      T& in_res, std::string_view in_mine_type, std::string_view in_access_control_allow_origin,
      chrono::seconds in_timeout
  ) const;
  template <typename T>
  void set_response_file_header(
      T& in_res, const FSys::path& in_path, std::string_view in_mine_type, bool in_has_cache_control,
      bool in_is_attachment, const std::string& in_attachment_filename, std::string_view in_access_control_allow_origin,
      chrono::seconds in_timeout
  ) const;
  static std::string zlib_compress(const std::string& in_str);

  http_body_base_t& set_status(boost::beast::http::status in_status);
  http_body_base_t& set_mine_type(std::string_view in_mine_type);
  http_body_base_t& set_deflate(bool in_deflate = true);
  http_body_base_t& set_cache_control(bool in_enable = true);
  http_body_base_t& set_attachment(std::string in_filename = {});
  http_body_base_t& set_header(boost::beast::http::field in_field, std::string in_value);
  http_body_base_t& add_header(boost::beast::http::field in_field, std::string in_value);

  [[nodiscard]] boost::beast::http::status get_status() const;
  [[nodiscard]] std::string_view get_mine_type() const;
  [[nodiscard]] bool has_cache_control() const;
  [[nodiscard]] bool is_deflate() const;
  [[nodiscard]] bool is_attachment() const;
  [[nodiscard]] const std::string& get_attachment_filename() const;
  [[nodiscard]] const std::vector<std::pair<boost::beast::http::field, std::string>>& get_headers() const;

  virtual boost::beast::http::message_generator make_message(const detail::session_data& in_session_data) const = 0;
};

using http_body_ptr = std::unique_ptr<http_body_base_t>;

struct http_body_json_t : http_body_base_t {
  nlohmann::json body_{};

  http_body_json_t() = default;
  explicit http_body_json_t(
      nlohmann::json in_body, boost::beast::http::status in_status = boost::beast::http::status::ok
  );

  boost::beast::http::message_generator make_message(const detail::session_data& in_session_data) const override;
};

struct http_body_string_t : http_body_base_t {
  std::string body_{};

  http_body_string_t() = default;
  explicit http_body_string_t(
      std::string in_body, std::string_view in_mime_type = "text/plain; charset=utf-8",
      boost::beast::http::status in_status = boost::beast::http::status::ok
  );

  boost::beast::http::message_generator make_message(const detail::session_data& in_session_data) const override;
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

  boost::beast::http::message_generator make_message(const detail::session_data& in_session_data) const override;
};

struct http_body_file_t : http_body_base_t {
  FSys::path path_{};
  http_file_range_mode range_mode_{http_file_range_mode::full};
  std::uint64_t range_begin_{0};
  std::uint64_t range_end_{0};
  bool use_deflate_body_{false};

  http_body_file_t() = default;
  explicit http_body_file_t(
      FSys::path in_path, std::string_view in_mime_type,
      boost::beast::http::status in_status = boost::beast::http::status::ok
  );

  http_body_file_t& set_range_mode(http_file_range_mode in_mode);
  http_body_file_t& set_range(std::uint64_t in_begin, std::uint64_t in_end);
  http_body_file_t& set_deflate_body(bool in_deflate = true);

  boost::beast::http::message_generator make_message(const detail::session_data& in_session_data) const override;
};

struct http_body_empty_t : http_body_base_t {
  http_body_empty_t();
  explicit http_body_empty_t(boost::beast::http::status in_status);

  boost::beast::http::message_generator make_message(const detail::session_data& in_session_data) const override;
};

struct http_body_error_t : http_body_base_t {
  std::string message_{};
  std::int32_t code_{-1};

  explicit http_body_error_t(boost::beast::http::status in_status, std::string in_message, std::int32_t in_code = -1);

  boost::beast::http::message_generator make_message(const detail::session_data& in_session_data) const override;
};

using http_body_variant = std::variant<
    http_body_json_t, http_body_string_t, http_body_binary_t, http_body_file_t, http_body_empty_t, http_body_error_t>;
}  // namespace doodle::http

#include <doodle_lib/core/http/http_session_data.h>

namespace doodle::http {

template <typename T>
void http_body_base_t::set_response_header(
    T& in_res, std::string_view in_mine_type, std::string_view in_access_control_allow_origin,
    chrono::seconds in_timeout
) const {
  auto l_time = chrono::floor<chrono::seconds>(chrono::system_clock::now());
  in_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  if (!in_mine_type.empty()) in_res.set(boost::beast::http::field::content_type, in_mine_type);
  in_res.set(boost::beast::http::field::access_control_allow_origin, in_access_control_allow_origin);
  in_res.set(boost::beast::http::field::access_control_allow_credentials, "true");
  in_res.set(boost::beast::http::field::access_control_allow_methods, "GET, POST, PUT, PATCH, DELETE, OPTIONS");
  in_res.set(
      boost::beast::http::field::access_control_allow_headers,
      "Authorization, Origin, X-Requested-With, Content-Type, Accept"
  );
  in_res.set(boost::beast::http::field::date, fmt::format("{:%a, %d %b %Y %T} GMT", l_time));
  in_res.set(boost::beast::http::field::vary, "Cookie");
  if (in_timeout != doodle_config::g_timeout)
    in_res.set(
        boost::beast::http::field::keep_alive,
        fmt::format("timeout={}, max={}", in_timeout.count(), doodle_config::g_max_keep_alive)
    );
}

template <typename T>
void http_body_base_t::set_response_file_header(
    T& in_res, const FSys::path& in_path, std::string_view in_mine_type, bool in_has_cache_control,
    bool in_is_attachment, const std::string& in_attachment_filename, std::string_view in_access_control_allow_origin,
    chrono::seconds in_timeout
) const {
  set_response_header(in_res, in_mine_type, in_access_control_allow_origin, in_timeout);
  auto l_time = chrono::floor<chrono::seconds>(chrono::system_clock::now());
  if (in_has_cache_control) {
    in_res.set(boost::beast::http::field::cache_control, "public,max-age=300");
    in_res.set(
        boost::beast::http::field::expires, fmt::format("{:%a, %d %b %Y %T} GMT", l_time + chrono::seconds{300})
    );
  }
  auto l_last_time =
      chrono::floor<chrono::seconds>(FSys::file_time_type::clock::to_utc(FSys::last_write_time(in_path)));
  in_res.set(boost::beast::http::field::last_modified, fmt::format("{:%a, %d %b %Y %T} GMT", l_last_time));
  in_res.set(boost::beast::http::field::etag, generate_etag(in_path));
  if (in_is_attachment) {
    in_res.set(
        boost::beast::http::field::content_disposition,
        fmt::format(
            "attachment; filename=\"{}\"",
            in_attachment_filename.empty() ? in_path.filename().string() : in_attachment_filename
        )
    );
  }
}

}  // namespace doodle::http
