//
// Created by td_main on 2023/8/3.
//

#pragma once
#include "doodle_core/core/global_function.h"

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/render_farm/detail/basic_json_body.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/hana.hpp>
#include <boost/signals2.hpp>
#include <boost/url.hpp>

#include <memory>
#include <nlohmann/json.hpp>
namespace doodle::render_farm {

namespace detail {
struct basic_json_body;

template <boost::beast::http::verb in_method>
class http_method {
 public:
  void run(const entt::handle& in_handle);
};

}  // namespace detail
/**
 * @brief 会话类 用于处理客户端的请求  一个会话对应一个客户端
 */
class working_machine_session {
 public:
  explicit working_machine_session(boost::asio::ip::tcp::socket in_socket) : stream_{std::move(in_socket)} {}

  void run();
  ~working_machine_session() { do_close(); }

  template <typename Error_Type>
  void send_error(const Error_Type& in_error) {
    boost::beast::http::response<detail::basic_json_body> l_response{boost::beast::http::status::ok, 11};
    l_response.body() = {{"state", boost::diagnostic_information(in_error)}, {"id", -1}};
    l_response.keep_alive(false);
    send_response(boost::beast::http::message_generator{std::move(l_response)});
  };
  template <typename Error_Type>
  void send_error_code(const Error_Type& in_error) {
    boost::beast::http::response<detail::basic_json_body> l_response{boost::beast::http::status::ok, 11};
    l_response.body() = {{"state", in_error.message()}, {"id", -1}};
    l_response.keep_alive(false);
    send_response(boost::beast::http::message_generator{std::move(l_response)});
  };

 private:
  template <boost::beast::http::verb in_method>
  friend class detail::http_method;

  template <typename>
  friend class async_read_body_op;

 private:
  void do_read();
  /**
   * @brief 解析请求,并返回响应
   * @param ec 错误码
   * @param bytes_transferred  读取的字节数
   */
  void on_parser(boost::system::error_code ec, std::size_t bytes_transferred);
  void on_write(bool keep_alive, boost::system::error_code ec, std::size_t bytes_transferred);
  void do_close();

  template <boost::beast::http::verb http_verb>
  void do_parser();

  boost::beast::flat_buffer buffer_;
  render_farm::working_machine_ptr working_machine_ptr_;

 public:
  void send_response(boost::beast::http::message_generator&& in_message_generator);
  boost::beast::tcp_stream stream_;
  boost::url url_;
  boost::beast::http::request_parser<boost::beast::http::empty_body> request_parser_;
  boost::signals2::scoped_connection connection_;
};

namespace detail {
template <boost::beast::http::verb in_method>
void http_method<in_method>::run(const entt::handle& in_handle) {
  boost::beast::http::response<boost::beast::http::empty_body> l_response{boost::beast::http::status::not_found, 11};
  in_handle.get<working_machine_session>().send_response(boost::beast::http::message_generator{std::move(l_response)});
}
}  // namespace detail
}  // namespace doodle::render_farm
