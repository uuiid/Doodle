//
// Created by td_main on 2023/8/3.
//

#include "working_machine.h"

#include <doodle_lib/render_farm/detail/url_route_get.h>
#include <doodle_lib/render_farm/detail/url_route_post.h>
#include <doodle_lib/render_farm/working_machine_session.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
namespace doodle::render_farm {

void working_machine::run() {
  //  acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  //  acceptor_.bind(end_point_);
  //  acceptor_.listen(boost::asio::socket_base::max_listen_connections);
  do_accept();
}
void working_machine::do_accept() {
  acceptor_.async_accept(
      boost::asio::make_strand(g_io_context()),
      boost::beast::bind_front_handler(&working_machine::on_accept, shared_from_this())
  );
}
void working_machine::on_accept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
  if (ec) {
    if (ec == boost::asio::error::operation_aborted) {
      return;
    }
    DOODLE_LOG_ERROR("on_accept error: {}", ec.what());
  } else {
    entt::handle{*g_reg(), g_reg()->create()}.emplace<working_machine_session>(std::move(socket)).run();
  }
  do_accept();
}
void working_machine::stop() {
  acceptor_.cancel();
  acceptor_.close();
}
void working_machine::config_server() {
  using http_get = detail::http_method<boost::beast::http::verb::get>;
  ctx().emplace<http_get>(http_get::make_server());
  using http_post = detail::http_method<boost::beast::http::verb::post>;
  ctx().emplace<http_post>(http_post::make_server());
  run();
}
void working_machine::config_client() {
  using http_get = detail::http_method<boost::beast::http::verb::get>;
  ctx().emplace<http_get>(http_get::make_client());
  using http_post = detail::http_method<boost::beast::http::verb::post>;
  ctx().emplace<http_post>(http_post::make_client());
  run();
}
void working_machine::config_work() {
  using http_get = detail::http_method<boost::beast::http::verb::get>;
  ctx().emplace<http_get>(http_get::make_work());
  using http_post = detail::http_method<boost::beast::http::verb::post>;
  ctx().emplace<http_post>(http_post::make_work());
  run();
}
}  // namespace doodle::render_farm