//
// Created by TD on 2024/2/20.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>

namespace doodle::http {
class http_websocket_client;
}
namespace doodle::http {
struct capture_t;
class http_route;
using http_route_ptr = std::shared_ptr<http_route>;
class http_session_data;
using http_session_data_ptr = std::shared_ptr<http_session_data>;
using executor_type       = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;
using endpoint_type       = boost::asio::ip::tcp::endpoint;
using tcp_stream_type     = executor_type::as_default_on_t<boost::beast::tcp_stream>;
using tcp_stream_type_ptr = std::shared_ptr<tcp_stream_type>;
using request_parser_ptr  = std::shared_ptr<boost::beast::http::request_parser<boost::beast::http::empty_body>>;
namespace detail {
enum class content_type {
  text_plain,
  application_json,
  text_html,
  text_css,
  text_javascript,
  text_xml,
  image_jpeg,
  image_jpg,
  image_png,
  image_gif,
  form_data,
};

class session_data {
 public:
  session_data()  = default;
  ~session_data() = default;
  logger_ptr logger_;
  std::shared_ptr<capture_t> capture_;
  http_route_ptr route_ptr_;
  boost::url url_;
  std::uint32_t version_{};
  bool keep_alive_{};

  content_type content_type_{content_type::text_plain};
  std::variant<std::string, nlohmann::json> body_;  // std::variant<std::string, nlohmann::json>
  // 请求头
  boost::beast::http::request_header<> req_header_;

  // 每次连接自定义数据
  std::any user_data_;


  boost::beast::http::message_generator make_error_code_msg(
      boost::beast::http::status in_status, const boost::system::error_code& ec, const std::string& in_str = ""
  ) {
    return make_error_code_msg(in_status, ec.what() + in_str);
  }

  boost::beast::http::message_generator make_error_code_msg(
      boost::beast::http::status in_code, const std::string& in_str, std::int32_t in_msg_code = -1
  );

  template <typename T = boost::beast::http::string_body, typename Value>
  boost::beast::http::response<T> make_msg(Value&& in_body) {
    boost::beast::http::response<T> l_res{boost::beast::http::status::ok, version_};
    l_res.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
    l_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    l_res.keep_alive(keep_alive_);
    l_res.body() = std::move(in_body);
    l_res.prepare_payload();
    return l_res;
  }
};

class http_websocket_data {
 public:
  nlohmann::json body_{};  // std::variant<std::string, nlohmann::json>
  logger_ptr logger_{};
  std::string remote_endpoint_{};
  std::shared_ptr<void> user_data_{};
  std::weak_ptr<http_websocket_client> client_{};
  void* in_args_{};
};

boost::asio::awaitable<void> async_session(boost::asio::ip::tcp::socket in_socket, http_route_ptr in_route_ptr);
}  // namespace detail
using session_data     = detail::session_data;
using session_data_ptr = std::shared_ptr<detail::session_data>;
}  // namespace doodle::http