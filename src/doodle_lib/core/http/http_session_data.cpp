//
// Created by TD on 2024/2/20.
//

#include "http_session_data.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/lib_warp/boost_fmt_url.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/http/socket_logger.h>
namespace doodle::http {

void http_session_data::rend_head() {
  stream_->expires_after(30s);
  entt::handle l_self_handle{*g_reg(), entt::to_entity(*g_reg(), *this)};
  request_parser_ = std::make_unique<boost::beast::http::request_parser<boost::beast::http::empty_body>>();
  boost::beast::http::async_read_header(
      *stream_, buffer_, *request_parser_, boost::beast::bind_front_handler(&http_session_data::do_read, this)
  );
}

void http_session_data::do_read(boost::system::error_code ec, std::size_t bytes_transferred) {
  entt::handle l_self_handle{*g_reg(), entt::to_entity(*g_reg(), *this)};
  auto l_logger = l_self_handle.get<socket_logger>().logger_;

  if (ec) {
    l_logger->log(log_loc(), level::err, fmt::format("读取头部失败 {}", ec.message()));
    do_close();
    return;
  }
  version_    = request_parser_->get().version();
  keep_alive_ = request_parser_->keep_alive();

  url_        = boost::url{request_parser_->get().target()};
  l_logger->log(log_loc(), level::info, fmt::format("开始解析 url {}", url_));
  auto& l_rote = l_self_handle.get<http_route>();
  if (auto l_ptr = l_rote(request_parser_->get().method(), url_.segments(), l_self_handle); l_ptr)
    l_ptr->callback_(l_self_handle);
  else {
    seed_error(
        boost::beast::http::status::not_found,
        boost::system::errc::make_error_code(boost::system::errc::no_such_file_or_directory)
    );
  }
}
void http_session_data::seed_error(boost::beast::http::status in_status, boost::system::error_code ec) {
  entt::handle l_self_handle{*g_reg(), entt::to_entity(*g_reg(), *this)};
  auto l_logger = l_self_handle.get<socket_logger>().logger_;
  l_logger->log(log_loc(), level::err, fmt::format("发送错误码 {}", ec.message()));

  boost::beast::http::response<boost::beast::http::string_body> l_response{in_status, version_};
  l_response.set(boost::beast::http::field::content_type, "plain/text");
  l_response.set(boost::beast::http::field::accept, "application/json");
  l_response.keep_alive(keep_alive_);
  l_response.body() = ec.message();
  l_response.prepare_payload();
  seed(std::move(l_response));
}
void http_session_data::seed(boost::beast::http::message_generator in_message_generator) {
  boost::beast::async_write(
      *stream_, std::move(in_message_generator), boost::beast::bind_front_handler(&http_session_data::do_send, this)
  );
}
void http_session_data::do_send(boost::system::error_code ec, std::size_t bytes_transferred) {
  entt::handle l_self_handle{*g_reg(), entt::to_entity(*g_reg(), *this)};
  auto l_logger = l_self_handle.get<socket_logger>().logger_;
  if (ec) {
    l_logger->log(log_loc(), level::err, fmt::format("发送错误码 {}", ec.message()));
    do_close();
    return;
  }
  if (!keep_alive_) {
    do_close();
    return;
  }
  buffer_.clear();
  stream_->expires_after(30s);
  rend_head();
}
void http_session_data::do_close() {
  boost::system::error_code l_error_code{};
  stream_->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, l_error_code);
  if (l_error_code) {
    default_logger_raw()->log(log_loc(), level::err, fmt::format("关闭 socket 失败 {}", l_error_code.message()));
  }
  entt::handle l_self_handle{*g_reg(), entt::to_entity(*g_reg(), *this)};

  boost::asio::post(g_io_context(), [l_self_handle] {
    auto l_h = l_self_handle;
    l_h.destroy();
  });
}

}  // namespace doodle::http