//
// Created by TD on 2024/2/20.
//

#pragma once

#include <doodle_core/configure/static_value.h>
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/http/http_content_type.h>
#include <doodle_lib/core/http/multipart_body_value.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>

#include <string>
#include <tl/expected.hpp>

namespace doodle::http {
struct multipart_body;
class http_websocket_client;
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
struct http_header_ctrl {
  boost::beast::http::status status_{boost::beast::http::status::ok};
  std::string_view mine_type_{"application/json; charset=utf-8"};
  bool has_cache_control_{false};
  bool is_deflate_{false};
  bool is_attachment_{false};
  std::string attachment_filename_{};
};
namespace detail {

class session_data : public std::enable_shared_from_this<session_data> {
  std::string zlib_compress(const std::string& in_str);

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
  chrono::seconds timeout_{doodle_config::g_timeout};
  void set_session();
  boost::asio::awaitable<bool> parse_body();
  boost::asio::awaitable<void> async_websocket_session();
  boost::asio::awaitable<void> save_bode_file(const std::string& in_ext);
  boost::asio::awaitable<void> save_multipart_form_data_file();
  std::string access_control_allow_origin_{"*"};

  template <typename T>
  auto set_response_header(T& in_res, std::string_view in_mine_type);

  template <typename T>
  auto set_response_file_header(T& in_res, const FSys::path& in_path, const http_header_ctrl& in_http_header_ctrl);

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

  content_type content_type_{content_type::text_plain};
  std::variant<std::string, nlohmann::json, FSys::path, multipart_body_impl::value_type_impl>
      body_;  // std::variant<std::string, nlohmann::json>
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
  // 检查请求头中是否包含 deflate 压缩字段
  bool is_deflate() const { return req_header_[boost::beast::http::field::accept_encoding].contains("deflate"); }
  boost::asio::awaitable<void> run();
  const boost::beast::http::verb& method() const { return method_verb_; }
  void set_access_control_allow_origin(const std::string& in_str) { access_control_allow_origin_ = in_str; }

  void async_run_detached();
  boost::beast::http::message_generator make_error_code_msg(
      boost::beast::http::status in_status, const boost::system::error_code& ec, const std::string& in_str = ""
  ) {
    return make_error_code_msg(in_status, ec.what() + in_str);
  }

  boost::beast::http::message_generator make_error_code_msg(
      boost::beast::http::status in_code, const std::string& in_str, std::int32_t in_msg_code = -1
  );

  boost::beast::http::response<boost::beast::http::string_body> make_msg(std::string&& in_body) {
    return make_msg(
        std::move(in_body),
        http_header_ctrl{
            .status_ = req_header_.method() == boost::beast::http::verb::post ? boost::beast::http::status::created
                                                                              : boost::beast::http::status::ok
        }
    );
  }

  boost::beast::http::response<boost::beast::http::string_body> make_msg(const nlohmann::json& in_body) {
    return make_msg(
        std::move(in_body.dump()),
        http_header_ctrl{
            .status_ = req_header_.method() == boost::beast::http::verb::post ? boost::beast::http::status::created
                                                                              : boost::beast::http::status::ok
        }
    );
  }
  boost::beast::http::response<boost::beast::http::string_body> make_msg_204() {
    return make_msg(std::string{}, http_header_ctrl{.status_ = boost::beast::http::status::no_content});
  }

  boost::beast::http::response<boost::beast::http::string_body> make_msg(
      std::string&& in_body, const http_header_ctrl& in_http_header_ctrl
  );
  template <typename Char>
  boost::beast::http::response<boost::beast::http::vector_body<Char>> make_msg(
      std::vector<Char>&& in_body, const http_header_ctrl& in_http_header_ctrl
  );

  template <typename Char>
  boost::beast::http::response<boost::beast::http::vector_body<Char>> make_msg(
      std::vector<Char>&& in_body, std::string_view in_http_header_ctrl
  ) {
    return make_msg(std::move(in_body), http_header_ctrl{.mine_type_ = in_http_header_ctrl});
  }
  boost::beast::http::message_generator make_msg(
      const FSys::path& in_path, const http_header_ctrl& in_http_header_ctrl
  );
  boost::beast::http::message_generator make_msg(const FSys::path& in_path, std::string_view in_http_header_ctrl) {
    return make_msg(in_path, http_header_ctrl{.mine_type_ = in_http_header_ctrl});
  }

 private:
  // boost::beast::http::response<zlib_deflate_file_body> make_file_deflate(
  //     const FSys::path& in_path, const http_header_ctrl& in_http_header_ctrl
  // );
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