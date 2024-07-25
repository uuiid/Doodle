//
// Created by TD on 2024/2/20.
//

#pragma once

#include <doodle_core/core/wait_op.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <doodle_lib/core/http/http_websocket_data.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>

namespace doodle::http {
struct capture_t;
class http_route;
using http_route_ptr = std::shared_ptr<http_route>;
class http_session_data;
using http_session_data_ptr = std::shared_ptr<http_session_data>;

namespace detail {
enum class content_type {
  text_plain,
  application_json,
  text_html,
  text_css,
  text_javascript,
  text_xml,
  image_jpeg,
  image_png,
  image_gif,
};

class session_data {
public:
  session_data()  = default;
  ~session_data() = default;
  logger_ptr logger_{};
  std::shared_ptr<capture_t> capture_{};
  http_route_ptr route_ptr_{};
  boost::url url_;
  std::shared_ptr<void> request_body_parser_;
  std::uint32_t version_{};
  bool keep_alive_{};

  content_type content_type_{content_type::text_plain};
  std::variant<std::string, nlohmann::json> body_{}; // std::variant<std::string, nlohmann::json>

  boost::beast::http::message_generator make_error_code_msg(
    boost::beast::http::status in_status, const boost::system::error_code& ec, const std::string& in_str = ""
  );
  boost::beast::http::message_generator make_error_code_msg(
    std::int32_t in_code, const std::string& in_str
  );

  inline boost::beast::http::message_generator make_error_code_msg(
    boost::beast::http::status in_code, const std::string& in_str
  ) {
    return make_error_code_msg(enum_to_num(in_code), in_str);
  }
};

class http_websocket_data {
public:
  nlohmann::json body_{}; // std::variant<std::string, nlohmann::json>
  logger_ptr logger_{};
};


boost::asio::awaitable<void> async_session(boost::asio::ip::tcp::socket in_socket, http_route_ptr in_route_ptr);
} // namespace detail
using session_data     = detail::session_data;
using session_data_ptr = std::shared_ptr<detail::session_data>;
} // namespace doodle::http