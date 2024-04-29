//
// Created by TD on 2024/2/20.
//

#include "http_session_data.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/lib_warp/boost_fmt_url.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/http/http_websocket_data.h>
#include <doodle_lib/core/http/socket_logger.h>
namespace doodle::http {

void http_session_data::rend_head() {
  stream_->expires_after(30s);
  request_parser_ = std::make_unique<boost::beast::http::request_parser<boost::beast::http::empty_body>>();
  buffer_.clear();
  boost::beast::http::async_read_header(
      *stream_, buffer_, *request_parser_,
      boost::beast::bind_front_handler(&http_session_data::do_read, shared_from_this())
  );
}

void http_session_data::do_read(boost::system::error_code ec, std::size_t bytes_transferred) {
  if (ec) {
    logger_->log(log_loc(), level::err, "读取头部失败 {}", ec);
    do_close();
    return;
  }
  if (!request_parser_->is_header_done()) {
    logger_->log(log_loc(), level::err, "读取头部不完整");
    do_close();
    return;
  }

  version_    = request_parser_->get().version();
  keep_alive_ = request_parser_->keep_alive();

  url_        = boost::url{request_parser_->get().target()};
  logger_->log(log_loc(), level::info, "开始解析 url {}", url_);
  auto& l_rote    = *route_ptr_;
  auto l_self_ptr = shared_from_this();
  l_rote(request_parser_->get().method(), url_.segments(), l_self_ptr)->callback_(l_self_ptr);
}
void http_session_data::seed_error(
    boost::beast::http::status in_status, boost::system::error_code ec, const std::string& in_str
) {
  logger_->log(log_loc(), level::err, "发送错误码 {} {}", ec, in_str);

  boost::beast::http::response<boost::beast::http::string_body> l_response{in_status, version_};
  l_response.set(boost::beast::http::field::content_type, "plain/text");
  l_response.set(boost::beast::http::field::accept, "application/json");
  l_response.set(boost::beast::http::field::access_control_allow_origin, "*");
  l_response.keep_alive(keep_alive_);
  l_response.body() = ec.message() + in_str;
  l_response.prepare_payload();
  seed(std::move(l_response));
}
void http_session_data::seed(boost::beast::http::message_generator in_message_generator) {
  logger_->log(log_loc(), level::err, "写入 {}", url_);
  boost::beast::async_write(
      *stream_, std::move(in_message_generator),
      boost::beast::bind_front_handler(&http_session_data::do_send, shared_from_this())
  );
}
void http_session_data::do_send(boost::system::error_code ec, std::size_t bytes_transferred) {
  if (ec) {
    logger_->log(log_loc(), level::err, "发送错误码 {}", ec);
    do_close();
    return;
  }
  if (!keep_alive_) {
    do_close();
    return;
  }
  if (!stream_) {
    logger_->log(log_loc(), level::err, "发送错误码 stream 为空");
    do_close();
    return;
  }
  stream_->expires_after(30s);
  rend_head();
}
void http_session_data::do_close() {
  boost::system::error_code l_error_code{};
  if (stream_) {
    stream_->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, l_error_code);
    if (l_error_code) {
      logger_->log(log_loc(), level::err, "关闭 socket 失败 {}", l_error_code);
    }
  }
}

namespace session {

// void http_method_web_socket::upgrade_websocket(const entt::handle& in_handle) const {
//   auto& l_read = in_handle.emplace_or_replace<session::async_read_body<boost::beast::http::string_body>>(in_handle);
//   l_read.async_end([](const boost::system::error_code& ec, const entt::handle& in_handle_1) {
//     auto l_logger = in_handle_1.get<socket_logger>().logger_;
//     if (ec == boost::beast::http::error::end_of_stream) {
//       in_handle_1.get<http_session_data>().do_close();
//       return;
//     }
//     if (ec) {
//       l_logger->log(log_loc(), level::err, "on_read error: {}", ec);
//       in_handle_1.get<http_session_data>().seed_error(
//           boost::beast::http::status::internal_server_error,
//           boost::system::errc::make_error_code(boost::system::errc::bad_message)
//       );
//       return;
//     }
//     boost::beast::get_lowest_layer(*in_handle_1.get<http_session_data>().stream_).expires_never();
//     in_handle_1.emplace<http_websocket_data>(std::move(*in_handle_1.get<http_session_data>().stream_)).run();
//     in_handle_1.erase<http_session_data>();
//   });
// }
//
// void http_method_web_socket::operator()(const entt::handle& in_handle) const {
//   if (boost::beast::websocket::is_upgrade(in_handle.get<http_session_data>().request_parser_->get())) {
//     upgrade_websocket(in_handle);
//     return;
//   } else {
//     set_handle_fun_(wait_op_, in_handle);
//     wait_op_->complete();
//   }
// }
}  // namespace session

}  // namespace doodle::http