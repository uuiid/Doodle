//
// Created by td_main on 2023/8/3.
//

#include "working_machine_session.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/bind_front_handler.h>
#include <doodle_lib/render_farm/detail/basic_json_body.h>
#include <doodle_lib/render_farm/detail/render_ue4.h>
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
  connection_          = doodle::app_base::Get().on_stop.connect([this]() { do_close(); });
  working_machine_ptr_ = doodle_lib::Get().ctx().get<render_farm::working_machine_ptr>();
  boost::asio::dispatch(
      boost::asio::make_strand(stream_.get_executor()),
      bind_reg_handler(&working_machine_session::do_read, g_reg(), this)
  );
}

template <boost::beast::http::verb http_verb>
void working_machine_session::do_parser() {
  stream_.expires_after(30s);
  make_handle(this);
  using http_method = detail::http_method<http_verb>;
  if (!working_machine_ptr_->ctx().contains<http_method>()) {
    boost::beast::http::response<boost::beast::http::string_body> l_response{boost::beast::http::status::not_found, 11};
    l_response.keep_alive(false);
    l_response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    l_response.set(boost::beast::http::field::content_type, "text/html");
    l_response.body() = fmt::format("The resource '{}' was not found.", request_parser().get().target());
    l_response.prepare_payload();
    send_response(std::move(l_response));
    return;
  }
  working_machine_ptr_->ctx().get<http_method>().run(make_handle(this));
}

void working_machine_session::do_read() {
  stream_.expires_after(30s);
  boost::beast::http::async_read_header(
      stream_, buffer_, request_parser_, bind_reg_handler(&working_machine_session::on_parser, g_reg(), this)
  );
}

void working_machine_session::on_parser(boost::system::error_code ec, std::size_t bytes_transferred) {
  if (ec) {
    DOODLE_LOG_ERROR("on_write error: {}", ec.message());
    //    if (ec == boost::beast::http::error::end_of_stream) {
    //      return;
    //    }
    do_close();
  }

  if (bytes_transferred == 0) {
    do_close();
    return;
  }
  url_ = boost::url{request_parser_.get().target()};
  try {
    switch (request_parser_.get().method()) {
      case boost::beast::http::verb::get:
        do_parser<boost::beast::http::verb::get>();
        break;
      case boost::beast::http::verb::head:
        do_parser<boost::beast::http::verb::head>();
        break;
      case boost::beast::http::verb::post: {
        do_parser<boost::beast::http::verb::post>();
        break;
      }
      default: {
        boost::beast::http::response<boost::beast::http::empty_body> l_response{
            boost::beast::http::status::not_found, 11};
        send_response(boost::beast::http::message_generator{std::move(l_response)});
        break;
      }
    }
  } catch (const doodle_error& e) {
    DOODLE_LOG_ERROR("doodle_error: {}", boost::diagnostic_information(e));
    boost::beast::http::response<boost::beast::http::string_body> l_response{boost::beast::http::status::not_found, 11};
    l_response.body() = e.what();
    send_response(boost::beast::http::message_generator{std::move(l_response)});
  }
  //  catch (const std::exception& e) {
  //    DOODLE_LOG_ERROR("std::exception: {}", e.what());
  //  }
}

void working_machine_session::send_response(boost::beast::http::message_generator&& in_message_generator) {
  const bool keep_alive = in_message_generator.keep_alive();

  boost::beast::async_write(
      stream_, std::move(in_message_generator),
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
  url_.clear();
  do_read();
}
void working_machine_session::do_close() {
  boost::system::error_code ec;
  stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
}

}  // namespace doodle::render_farm