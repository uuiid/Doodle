//
// Created by TD on 2024/2/20.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/http/http_content_type.h>
#include <doodle_lib/core/http/multipart_body.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>

#include <tl/expected.hpp>

namespace doodle::http {
class http_websocket_client;
struct capture_t;
class http_route;
class http_function;
class http_function_base_t;
using http_function_ptr = std::shared_ptr<http_function_base_t>;
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
  static constexpr auto g_body_limit{500 * 1024 * 1024};  // 500M

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

 public:
  session_data() = default;
  explicit session_data(boost::asio::ip::tcp::socket in_socket, http_route_ptr in_route_ptr);
  ~session_data() = default;
  http_route_ptr route_ptr_;
  logger_ptr logger_;
  std::shared_ptr<void> capture_;
  boost::url url_;
  std::uint32_t version_{};
  bool keep_alive_{};

  content_type content_type_{content_type::text_plain};
  std::variant<std::string, nlohmann::json, FSys::path, multipart_body::value_type>
      body_;  // std::variant<std::string, nlohmann::json>
  // 请求头
  boost::beast::http::request_header<> req_header_;

  // 每次连接自定义数据
  std::any user_data_;
  // 会产生判断内容为 application_json 或者 multipart_form_data 时, 转换为json格式
  nlohmann::json get_json() const;
  // 当使用 multipart_form_data 时, 获取上传的文件路径
  std::vector<FSys::path> get_files() const;
  // 检查请求头中是否包含 deflate 压缩字段
  bool is_deflate() const { return req_header_[boost::beast::http::field::accept_encoding].contains("deflate"); }
  boost::asio::awaitable<void> run();

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
        std::move(in_body), "application/json; charset=utf-8",
        req_header_.method() == boost::beast::http::verb::post ? boost::beast::http::status::created
                                                               : boost::beast::http::status::ok
    );
  }

  boost::beast::http::response<boost::beast::http::string_body> make_msg(const nlohmann::json& in_body) {
    return make_msg(
        std::move(in_body.dump()), "application/json; charset=utf-8",
        req_header_.method() == boost::beast::http::verb::post ? boost::beast::http::status::created
                                                               : boost::beast::http::status::ok
    );
  }
  boost::beast::http::response<boost::beast::http::string_body> make_msg_204() {
    return make_msg(std::string{}, "application/json; charset=utf-8", boost::beast::http::status::no_content);
  }

  boost::beast::http::response<boost::beast::http::string_body> make_msg(
      std::string&& in_body, boost::beast::http::status in_status
  ) {
    return make_msg(std::move(in_body), "application/json; charset=utf-8", in_status);
  }

  boost::beast::http::response<boost::beast::http::string_body> make_msg(
      std::string&& in_body, const std::string_view& mine_type, boost::beast::http::status in_status
  );
  template <typename Char>
  boost::beast::http::response<boost::beast::http::vector_body<Char>> make_msg(
      std::vector<Char>&& in_body, const std::string_view& mine_type, boost::beast::http::status in_status
  ) {
    boost::beast::http::response<boost::beast::http::vector_body<Char>> l_res{in_status, version_};
    l_res.set(boost::beast::http::field::content_type, mine_type);
    l_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    l_res.set(boost::beast::http::field::access_control_allow_origin, "*");
    l_res.set(boost::beast::http::field::access_control_allow_credentials, "true");
    l_res.set(boost::beast::http::field::access_control_allow_methods, "*");
    l_res.set(boost::beast::http::field::access_control_allow_headers, "*");
    l_res.keep_alive(keep_alive_);
    l_res.body() = std::move(in_body);
    l_res.prepare_payload();
    return l_res;
  }

  boost::beast::http::message_generator make_msg(const FSys::path& in_path, const std::string_view& mine_type);

  boost::beast::http::response<boost::beast::http::file_body> make_file(
      const FSys::path& in_path, const std::string_view& mine_type
  );
  boost::beast::http::response<zlib_deflate_file_body> make_file_deflate(
      const FSys::path& in_path, const std::string_view& mine_type
  );
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