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

  url_ = boost::url{request_parser_->get().target()};
  l_logger->log(log_loc(), level::info, fmt::format("开始解析 url {}", url_));
  auto& L_rote = l_self_handle.get<http_route>();
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