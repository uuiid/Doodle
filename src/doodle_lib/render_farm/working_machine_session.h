//
// Created by td_main on 2023/8/3.
//

#pragma once
#include "doodle_core/core/global_function.h"

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/hana.hpp>
#include <boost/signals2.hpp>

#include <memory>
#include <nlohmann/json.hpp>
namespace doodle::render_farm {

class working_machine;
namespace detail {
struct basic_json_body;

template <boost::beast::http::verb in_method>
class http_method {
 public:
  void run(std::shared_ptr<working_machine_session> in_session) {
    boost::beast::http::response<boost::beast::http::empty_body> l_response{boost::beast::http::status::not_found, 11};
    in_session->send_response(boost::beast::http::message_generator{std::move(l_response)});
  };
};
;
}  // namespace detail
/**
 * @brief 会话类 用于处理客户端的请求  一个会话对应一个客户端
 */
class working_machine_session : public std::enable_shared_from_this<working_machine_session> {
 public:
  explicit working_machine_session(
      boost::asio::ip::tcp::socket in_socket, std::shared_ptr<working_machine> in_working_machine
  )
      : stream_{std::move(in_socket)}, working_machine_{std::move(in_working_machine)} {}

  void run();
  ~working_machine_session() { do_close(); }

  struct function {
    boost::signals2::signal<boost::beast::http::response<detail::basic_json_body>(const nlohmann::json&)> on_submit_job;
    boost::signals2::signal<boost::beast::http::message_generator()> on_get_all_job;
  };

 private:
  template <boost::beast::http::verb in_method>
  friend class detail::http_method;

 private:
  void do_read();
  /**
   * @brief 解析请求,并返回响应
   * @param ec 错误码
   * @param bytes_transferred  读取的字节数
   */
  void on_parser(boost::system::error_code ec, std::size_t bytes_transferred);
  void send_response(boost::beast::http::message_generator&& in_message_generator);
  void on_write(bool keep_alive, boost::system::error_code ec, std::size_t bytes_transferred);
  void do_close();

  template <boost::beast::http::verb http_verb>
  void do_parser();

  boost::beast::tcp_stream stream_;
  boost::beast::flat_buffer buffer_;
  boost::beast::http::request_parser<boost::beast::http::empty_body> request_parser_;
  std::shared_ptr<working_machine> working_machine_;
  boost::signals2::scoped_connection connection_;
};
}  // namespace doodle::render_farm
