//
// Created by TD on 2024/2/20.
//

#include "http_session.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/lib_warp/boost_fmt_url.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/socket_logger.h>

namespace doodle::http {

void http_session::run() {}
void http_session::do_read(boost::system::error_code ec) {
  auto& l_data  = handle_.get<http_session_data>();
  auto l_logger = handle_.get<socket_logger>().logger_;

  auto& l_req   = *handle_.get<session::request_parser_empty_body>();
  auto& L_rote  = handle_.get<http_route>();
  l_data.url_   = boost::url{l_req.get().target()};
  l_logger->log(log_loc(), level::info, fmt::format("开始解析 url {}", l_data.url_));
  try {
    auto l_call = L_rote(l_req.get().method(), l_data.url_.segments(), handle_);
    if (l_call) {
      (*l_call)(handle_);
      return;
    } else {
      goto err_tag;
    }
  } catch (const doodle_error& e) {
    log_error(l_logger, fmt::format("doodle_error: {}", boost::diagnostic_information(e)));
    goto err_tag;
  }

err_tag:
  BOOST_BEAST_ASSIGN_EC(ec, error_enum::not_find_work_class);
  //  do_write::send_error_code(handle_, ec, boost::beast::http::status::internal_server_error);
}

void http_session::operator()(boost::system::error_code ec, std::size_t /*bytes_transferred*/) {
  if (!handle_) {
    default_logger_raw()->log(log_loc(), level::err, "无效的句柄");
    return;
  }

  handle_.get<http_session_data>()->expires_after(30s);
  auto l_logger = handle_.get<socket_logger>().logger_;
  if (ec) {
    if (ec != boost::beast::http::error::end_of_stream) {
      l_logger->log(log_loc(), level::err, fmt::format("on_read error: {} ", ec));
    } else {
      l_logger->log(log_loc(), level::warn, fmt::format("on_read error: {} ", ec));
    }
    //    do_close();
    return;
  }

  do_read(ec);
}

}  // namespace doodle::http