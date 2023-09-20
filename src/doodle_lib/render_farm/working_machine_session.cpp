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

namespace detail {
// template <typename Json_Conv_to>

//  检查url

// http 方法

}  // namespace detail

void working_machine_session::run() {
  ptr_->connection_          = doodle::app_base::Get().on_stop.connect([this]() { do_close(); });
  ptr_->working_machine_ptr_ = g_ctx().get<render_farm::working_machine_ptr>();
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
      log_error(ptr_->logger_, fmt::format("末端的流, 主动关闭 {} ", ec));
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
      goto end_tag;
    }
    auto l_call = (*ptr_->route_ptr_)(ptr_->request_parser_->get().method(), ptr_->url_.segments());
    if (l_call) {
      l_call(make_handle(this));
    } else {
      goto end_tag;
    }
  } catch (const doodle_error& e) {
    log_error(ptr_->logger_, fmt::format("doodle_error: {}", boost::diagnostic_information(e)));
    goto end_tag;
  }

end_tag:
  BOOST_BEAST_ASSIGN_EC(ec, error_enum::not_find_work_class);
  send_error_code(ec, boost::beast::http::status::internal_server_error);
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