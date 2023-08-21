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

/**
 * @brief 会话类 用于处理客户端的请求  一个会话对应一个客户端
 */
class working_machine_session {
 public:
  explicit working_machine_session(boost::asio::ip::tcp::socket in_socket, http_route_ptr in_route_ptr_)
      : ptr_(std::make_shared<data_type>(std::move(in_socket))) {
    ptr_->route_ptr_ = std::move(in_route_ptr_);
  }

  void run();
  ~working_machine_session() { do_close(); }

  template <typename Error_Type>
  void send_error(
      const Error_Type& in_error, boost::beast::http::status in_status = boost::beast::http::status::bad_request
  ) {
    boost::beast::http::response<detail::basic_json_body> l_response{in_status, 11};
    l_response.body() = {{"state", boost::diagnostic_information(in_error)}, {"id", -1}};
    l_response.keep_alive(false);
    send_response(boost::beast::http::message_generator{std::move(l_response)});
  };
  template <typename Error_Type>
  void send_error_code(
      const Error_Type& in_error, boost::beast::http::status in_status = boost::beast::http::status::bad_request
  ) {
    boost::beast::http::response<detail::basic_json_body> l_response{in_status, 11};
    l_response.body() = {{"state", in_error.message()}, {"id", -1}};
    l_response.keep_alive(false);
    send_response(boost::beast::http::message_generator{std::move(l_response)});
  };

 private:
  void do_read();
  /**
   * @brief 解析请求,并返回响应
   * @param ec 错误码
   * @param bytes_transferred  读取的字节数
   */
  void on_parser(boost::system::error_code ec, std::size_t bytes_transferred);
  void on_write(bool keep_alive, boost::system::error_code ec, std::size_t bytes_transferred);

  using request_parser_type     = boost::beast::http::request_parser<boost::beast::http::empty_body>;
  using request_parser_type_ptr = std::shared_ptr<request_parser_type>;

  struct data_type {
    data_type(boost::asio::ip::tcp::socket in_socket) : stream_(std::move(in_socket)) {}
    boost::beast::tcp_stream stream_;

    render_farm::working_machine_ptr working_machine_ptr_;
    boost::beast::flat_buffer buffer_;
    request_parser_type_ptr request_parser_;
    boost::url url_;
    boost::signals2::scoped_connection connection_;
    http_route_ptr route_ptr_;
  };
  std::shared_ptr<data_type> ptr_;

 public:
  void send_response(boost::beast::http::message_generator&& in_message_generator);
  void do_close();
  [[nodiscard("")]] inline boost::beast::http::request_parser<boost::beast::http::empty_body>& request_parser() {
    return *ptr_->request_parser_;
  };
  [[nodiscard("")]] inline boost::beast::tcp_stream& stream() { return ptr_->stream_; }
  // buffer
  [[nodiscard("")]] inline boost::beast::flat_buffer& buffer() { return ptr_->buffer_; }
  [[nodiscard("")]] inline const boost::beast::flat_buffer& buffer() const { return ptr_->buffer_; }
  // url
  [[nodiscard("")]] inline boost::url& url() { return ptr_->url_; }
  [[nodiscard("")]] inline const boost::url& url() const { return ptr_->url_; }
};

}  // namespace doodle::render_farm
