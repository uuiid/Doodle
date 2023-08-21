//
// Created by td_main on 2023/8/3.
//

#include "working_machine_session.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/bind_front_handler.h>
#include <doodle_lib/render_farm/detail/basic_json_body.h>
#include <doodle_lib/render_farm/detail/render_ue4.h>
#include <doodle_lib/render_farm/detail/url_route_base.h>
#include <doodle_lib/render_farm/detail/url_route_get.h>
#include <doodle_lib/render_farm/detail/url_route_post.h>
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
  ptr_->working_machine_ptr_ = doodle_lib::Get().ctx().get<render_farm::working_machine_ptr>();
  boost::asio::dispatch(
      boost::asio::make_strand(stream().get_executor()),
      bind_reg_handler(&working_machine_session::do_read, g_reg(), this)
  );
}

void working_machine_session::do_read() {
  stream().expires_after(30s);
  ptr_->request_parser_ = std::make_shared<request_parser_type>();

  boost::beast::http::async_read_header(
      stream(), buffer(), *ptr_->request_parser_, bind_reg_handler(&working_machine_session::on_parser, g_reg(), this)
  );
}

void working_machine_session::on_parser(boost::system::error_code ec, std::size_t bytes_transferred) {
  if (ec) {
    DOODLE_LOG_ERROR("on_write error: {}", ec.message());
    //    if (ec == boost::beast::http::error::end_of_stream) {
    //      return;
    //    }
    do_close();
    return;
  }

  ptr_->url_ = boost::url{ptr_->request_parser_->get().target()};
  DOODLE_LOG_INFO("开始解析 uel {}", ptr_->url_.path());
  try {
    auto l_has_call = (*ptr_->route_ptr_)(ptr_->request_parser_->get().method(), make_handle(this));
    if (!l_has_call) {
      boost::beast::http::response<boost::beast::http::empty_body> l_response{
          boost::beast::http::status::not_found, 11};
      send_response(boost::beast::http::message_generator{std::move(l_response)});
    }
  } catch (const doodle_error& e) {
    DOODLE_LOG_ERROR("doodle_error: {}", boost::diagnostic_information(e));
    boost::beast::http::response<boost::beast::http::string_body> l_response{boost::beast::http::status::not_found, 11};
    l_response.body() = e.what();
    send_response(boost::beast::http::message_generator{std::move(l_response)});
  }
}

void working_machine_session::send_response(boost::beast::http::message_generator&& in_message_generator) {
  const bool keep_alive = in_message_generator.keep_alive();

  boost::beast::async_write(
      stream(), std::move(in_message_generator),
      bind_reg_handler(&working_machine_session::on_write, g_reg(), this, keep_alive)
  );
}
void working_machine_session::on_write(bool keep_alive, boost::system::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);

  if (ec) {
    DOODLE_LOG_ERROR("on_write error: {}", ec.message());
    return;
  }

  if (!keep_alive) {
    return do_close();
  }
  ptr_->buffer_.clear();
  ptr_->url_.clear();
  ptr_->request_parser_ = std::make_shared<request_parser_type>();
  do_read();
}
void working_machine_session::do_close() {
  boost::system::error_code ec;
  ptr_->stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

  if (ec) {
    DOODLE_LOG_ERROR("do_close error: {}", ec.message());
  }
}

}  // namespace doodle::render_farm