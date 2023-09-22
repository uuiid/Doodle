//
// Created by td_main on 2023/8/3.
//

#include "working_machine_session.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/boost_fmt_asio.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/lib_warp/boost_fmt_url.h>

#include <doodle_lib/core/bind_front_handler.h>
#include <doodle_lib/render_farm/detail/basic_json_body.h>
#include <doodle_lib/render_farm/detail/render_ue4.h>
#include <doodle_lib/render_farm/detail/url_route_base.h>
#include <doodle_lib/render_farm/detail/url_route_get.h>
#include <doodle_lib/render_farm/detail/url_route_post.h>
#include <doodle_lib/render_farm/render_farm_fwd.h>
#include <doodle_lib/render_farm/websocket.h>
#include <doodle_lib/render_farm/working_machine.h>

#include "boost/url/urls.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/url.hpp>
namespace doodle::render_farm {

namespace session {
void do_read::run() {
  if (handle_ && handle_.all_of<working_machine_session_data>()) {
    handle_.get<working_machine_session_data>().stream_.expires_after(30s);
    handle_.emplace_or_replace<request_parser_empty_body>();
    boost::beast::http::async_read_header(
        handle_.get<working_machine_session_data>().stream_, handle_.get<working_machine_session_data>().buffer_,
        *handle_.get<request_parser_empty_body>(), std::move(*this)
    );
  }
}
void do_read::operator()(boost::system::error_code ec, std::size_t bytes_transferred) {
  if (!handle_) {
    log_error(fmt::format("无效的句柄"));
    return;
  }

  auto& l_data = handle_.get<working_machine_session_data>();
  boost::ignore_unused(bytes_transferred);
  l_data.stream_.expires_after(30s);
  auto l_logger = handle_.get<socket_logger>().logger_;
  if (ec) {
    if (ec != boost::beast::http::error::end_of_stream) {
      log_error(l_logger, fmt::format("on_write error: {} ", ec));
    } else {
      log_warn(l_logger, fmt::format("末端的流, 主动关闭 {} ", ec));
    }
    do_close{handle_};
    return;
  }

  auto& l_req  = *handle_.get<request_parser_empty_body>();
  auto& L_rote = handle_.get<detail::http_route>();
  l_data.url_  = boost::url{l_req.get().target()};
  log_info(l_logger, fmt::format("开始解析 uel {}", l_data.url_));
  try {
    if (boost::beast::websocket::is_upgrade(l_req.get())) {
      auto l_call = L_rote(l_data.url_.segments());
      if (l_call) {
        boost::beast::get_lowest_layer(l_data.stream_).expires_never();
        l_call(handle_);
        return;
      }
      goto err_tag;
    }
    auto l_call = L_rote(l_req.get().method(), l_data.url_.segments());
    if (l_call) {
      l_call(handle_);
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
  do_write::send_error_code(handle_, ec, boost::beast::http::status::internal_server_error);
}

void do_close::run() {
  if (handle_ && handle_.all_of<working_machine_session_data>()) {
    logger_ = handle_.get<socket_logger>().logger_;
    handle_.get<working_machine_session_data>().stream_.socket().shutdown(
        boost::asio::ip::tcp::socket::shutdown_send, ec
    );
    boost::asio::post(g_io_context(), std::move(*this));
  }
}

void do_close::operator()() {
  handle_.destroy();
  if (ec) {
    log_error(logger_, fmt::format("on_write error: {} ", ec));
  }
}

void do_write::run() {
  if (handle_ && handle_.all_of<working_machine_session_data>()) {
    auto& l_data = handle_.get<working_machine_session_data>();
    l_data.stream_.expires_after(30s);
    keep_alive_ = message_generator_.keep_alive();
    boost::beast::async_write(l_data.stream_, std::move(message_generator_), std::move(*this));
  }
}
void do_write::send_error_code(
    entt::handle in_handle, boost::system::error_code ec, boost::beast::http::status in_status
) {
  boost::beast::http::response<boost::beast::http::string_body> l_response{in_status, 11};
  l_response.set(boost::beast::http::field::content_type, "plain/text");
  l_response.set(boost::beast::http::field::accept, "application/json");
  l_response.body() = ec.message();
  l_response.prepare_payload();
  do_write{in_handle, std::move(l_response)}.run();
}

void do_write::operator()(boost::system::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (!handle_) {
    log_error(fmt::format("无效的句柄"));
    return;
  }
  auto& l_data  = handle_.get<working_machine_session_data>();
  auto l_logger = handle_.get<socket_logger>().logger_;
  if (ec) {
    log_error(l_logger, fmt::format("on_write error: {} ", ec));
    return;
  }
  if (!keep_alive_) {
    return do_close{handle_}.run();
  }
  l_data.stream_.expires_after(30s);
  do_read{handle_}.run();
}

}  // namespace session
void working_machine_session::run() {
  ptr_->connection_ = doodle::app_base::Get().on_stop.connect([this]() { do_close(); });
  boost::asio::dispatch(
      boost::asio::make_strand(stream().get_executor()),
      bind_reg_handler(&working_machine_session::do_read, g_reg(), this)
  );
  ptr_->logger_ = make_handle(this).get<socket_logger>().logger_;
}

void working_machine_session::do_read() {
  stream().expires_after(30s);
  ptr_->request_parser_ = std::make_shared<request_parser_type>();

  boost::beast::http::async_read_header(
      stream(), buffer(), *ptr_->request_parser_, bind_reg_handler(&working_machine_session::on_parser, g_reg(), this)
  );
}

void working_machine_session::on_parser(boost::system::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  stream().expires_after(30s);
  if (ec) {
    if (ec != boost::beast::http::error::end_of_stream) {
      log_error(ptr_->logger_, fmt::format("on_write error: {} ", ec));
    } else {
      log_warn(ptr_->logger_, fmt::format("末端的流, 主动关闭 {} ", ec));
    }
    do_close();
    return;
  }

  ptr_->url_ = boost::url{ptr_->request_parser_->get().target()};
  log_info(ptr_->logger_, fmt::format("开始解析 uel {}", ptr_->url_));
  try {
    if (boost::beast::websocket::is_upgrade(ptr_->request_parser_->get())) {
      auto l_call = (*ptr_->route_ptr_)(ptr_->url_.segments());
      if (l_call) {
        boost::beast::get_lowest_layer(ptr_->stream_).expires_never();
        l_call(make_handle(this));
        return;
      }
      goto err_tag;
    }
    auto l_call = (*ptr_->route_ptr_)(ptr_->request_parser_->get().method(), ptr_->url_.segments());
    if (l_call) {
      l_call(make_handle(this));
      goto end_tag;
    } else {
      goto err_tag;
    }
  } catch (const doodle_error& e) {
    log_error(ptr_->logger_, fmt::format("doodle_error: {}", boost::diagnostic_information(e)));
    goto err_tag;
  }

err_tag:
  BOOST_BEAST_ASSIGN_EC(ec, error_enum::not_find_work_class);
  send_error_code(ec, boost::beast::http::status::internal_server_error);
end_tag:
  stream().expires_after(30s);
}

void working_machine_session::send_response(boost::beast::http::message_generator&& in_message_generator) {
  const bool keep_alive = in_message_generator.keep_alive();
  stream().expires_after(30s);
  boost::beast::async_write(
      stream(), std::move(in_message_generator),
      bind_reg_handler(&working_machine_session::on_write, g_reg(), this, keep_alive)
  );
}
void working_machine_session::on_write(bool keep_alive, boost::system::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);

  if (ec) {
    log_error(ptr_->logger_, fmt::format("on_write error: {}", ec));
    return;
  }

  if (!keep_alive) {
    return do_close();
  }
  ptr_->buffer_.clear();
  ptr_->url_.clear();
  ptr_->request_parser_ = std::make_shared<request_parser_type>();
  stream().expires_after(30s);
  do_read();
}
void working_machine_session::do_close() {
  boost::system::error_code ec;
  if (ptr_->stream_.socket().is_open())
    ptr_->stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
  else
    log_warn(ptr_->logger_, fmt::format("socket is not open "));

  if (ec) {
    log_error(ptr_->logger_, fmt::format("do_close error: {}", ec));
  }
}

}  // namespace doodle::render_farm