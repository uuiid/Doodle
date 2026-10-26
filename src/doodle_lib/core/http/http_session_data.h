//
// Created by TD on 2024/2/20.
//

#pragma once

#include <doodle_core/configure/static_value.h>

#include <doodle_lib/core/http/http_body.h>
#include <doodle_lib/core/http/http_content_type.h>
#include <doodle_lib/core/http/multipart_body_value.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>

#include <string>

namespace doodle::http {
struct multipart_body;
struct capture_t;
class http_route;
class http_function;
using http_function_ptr = std::shared_ptr<http_function>;
struct zlib_deflate_file_body;
using http_route_ptr = std::shared_ptr<http_route>;
class http_session_data;
using http_session_data_ptr = std::shared_ptr<http_session_data>;
using executor_type         = boost::asio::use_awaitable_t<>;
using endpoint_type         = boost::asio::ip::tcp::endpoint;
using tcp_stream_type       = executor_type::as_default_on_t<boost::beast::tcp_stream>;
using tcp_stream_type_ptr   = std::shared_ptr<tcp_stream_type>;
using request_parser_ptr    = std::shared_ptr<boost::beast::http::request_parser<boost::beast::http::empty_body>>;
namespace detail {

class session_data : public std::enable_shared_from_this<session_data> {
  using empty_body_type               = boost::beast::http::empty_body;
  using empty_request_parser_type     = boost::beast::http::request_parser<empty_body_type>;
  using string_request_parser_type    = boost::beast::http::request_parser<boost::beast::http::string_body>;
  using file_request_parser_type      = boost::beast::http::request_parser<boost::beast::http::file_body>;
  using multipart_request_parser_type = boost::beast::http::request_parser<doodle::http::multipart_body>;

  using empty_request_parser_ptr      = std::shared_ptr<empty_request_parser_type>;
  using string_request_parser_ptr     = std::shared_ptr<string_request_parser_type>;
  using file_request_parser_ptr       = std::shared_ptr<file_request_parser_type>;
  using multipart_request_parser_ptr  = std::shared_ptr<multipart_request_parser_type>;
  static constexpr auto g_body_limit{800ll * 1024 * 1024 * 1024};  // 800G
  // 超时
  std::unique_ptr<tcp_stream_type> stream_;
  std::variant<
      empty_request_parser_ptr, string_request_parser_ptr, file_request_parser_ptr, multipart_request_parser_ptr>
      request_parser_;
  http_function_ptr callback_;
  boost::beast::flat_buffer buffer_;
  boost::beast::http::verb method_verb_{};
  void set_session();
  boost::asio::awaitable<bool> parse_body();
  boost::asio::awaitable<void> async_websocket_session();
  boost::asio::awaitable<void> save_bode_file(const std::string& in_ext);
  boost::asio::awaitable<void> save_multipart_form_data_file();
  std::variant<std::string, nlohmann::json, FSys::path, multipart_body_impl::value_type_impl>
      body_;  // std::variant<std::string, nlohmann::json>

  //  http 计数守卫
  http_connection_guard http_connection_guard_{};

 public:
  session_data() = default;
  explicit session_data(boost::asio::ip::tcp::socket in_socket, http_route_ptr in_route_ptr);
  ~session_data();
  http_route_ptr route_ptr_;
  logger_ptr logger_;
  std::shared_ptr<void> capture_;
  boost::url url_;
  std::uint32_t version_{};
  bool keep_alive_{};
  chrono::seconds timeout_{doodle_config::g_timeout};
  std::string access_control_allow_origin_{"*"};

  content_type content_type_{content_type::text_plain};
  // 请求头
  boost::beast::http::request_header<> req_header_;

  // 每次连接自定义数据
  std::any user_data_;
  // 会产生判断内容为 application_json 或者 multipart_form_data 时, 转换为json格式
  nlohmann::json get_json() const;
  // 当使用 multipart_form_data 时, 获取上传的文件路径
  std::vector<FSys::path> get_files() const;
  // 获取请求中的图片, 或者视频
  FSys::path get_file() const;
  // 直接获取字符串
  std::string get_string() const;
  // 检查请求头中是否包含 deflate 压缩字段
  bool is_deflate() const { return req_header_[boost::beast::http::field::accept_encoding].contains("deflate"); }
  boost::asio::awaitable<void> run();
  const boost::beast::http::verb& method() const { return method_verb_; }
  void set_access_control_allow_origin(const std::string& in_str) { access_control_allow_origin_ = in_str; }

  void async_run_detached();

  boost::beast::http::message_generator make_message(const http_body_ptr& in_body) const;
};

class http_websocket_data {
 public:
  nlohmann::json body_{};  // std::variant<std::string, nlohmann::json>
  logger_ptr logger_{};
  std::string remote_endpoint_{};
  std::shared_ptr<void> user_data_{};
  void* in_args_{};
};

boost::asio::awaitable<void> async_session(boost::asio::ip::tcp::socket in_socket, http_route_ptr in_route_ptr);
}  // namespace detail
using session_data     = detail::session_data;
using session_data_ptr = std::shared_ptr<detail::session_data>;
}  // namespace doodle::http
