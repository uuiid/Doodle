//
// Created by td_main on 2023/8/3.
//

#include "working_machine.h"

#include <doodle_app/app/app_command.h>

#include <doodle_lib/render_farm/detail/computer_manage.h>
#include <doodle_lib/render_farm/detail/ue_task_manage.h>
#include <doodle_lib/render_farm/detail/url_route_base.h>
#include <doodle_lib/render_farm/detail/url_route_get.h>
#include <doodle_lib/render_farm/detail/url_route_post.h>
#include <doodle_lib/render_farm/detail/url_route_put.h>
#include <doodle_lib/render_farm/detail/url_webscoket.h>
#include <doodle_lib/render_farm/render_farm_fwd.h>
#include <doodle_lib/render_farm/working_machine_session.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
namespace doodle::render_farm {

void working_machine::run() {
  //  acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  //  acceptor_.bind(end_point_);
  //  acceptor_.listen(boost::asio::socket_base::max_listen_connections);
  route_ptr_ = std::make_shared<detail::http_route>();
  (*route_ptr_)
      .get("v1/render_farm", detail::get_root_type{})
      .get("v1/render_farm/log/{handle}", detail::get_log_type_get{})
      .get("v1/render_farm/err/{handle}", detail::get_err_type_get{})
      .get("v1/render_farm/render_job", detail::render_job_type_get{})
      .get("v1/render_farm/computer", detail::computer_reg_type_get{})
      .get("v1/render_farm/repository", detail::repository_type_get{})
      .post<boost::beast::http::string_body>("v1/render_farm/render_job", detail::render_job_type_post{})
      .post<boost::beast::http::string_body>("v1/render_farm/log/{handle}", detail::get_log_type_post{})
      .post<boost::beast::http::string_body>("v1/render_farm/log/{handle}", detail::get_err_type_post{})
      .put<boost::beast::http::string_body>("v1/render_farm/render_job/{handle}", detail::render_job_type_put{})
      //      .put("v1/render_farm/render_job/{handle}", detail::render_job_type_put{});
      ;

  g_reg()->ctx().emplace<ue_task_manage>().run();
  g_reg()->ctx().emplace<computer_manage>().run();
  do_accept();
  signal_set_.async_wait([&](boost::system::error_code ec, int signal) {
    if (ec) {
      DOODLE_LOG_ERROR("signal_set_ error: {}", ec.message());
      return;
    }
    DOODLE_LOG_INFO("signal_set_ signal: {}", signal);
    this->stop();
    //    app_base::Get().stop_app();
  });
}
void working_machine::do_accept() {
  acceptor_.async_accept(
      boost::asio::make_strand(g_io_context()), boost::beast::bind_front_handler(&working_machine::on_accept, this)
  );
}
void working_machine::on_accept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
  if (ec) {
    if (ec == boost::asio::error::operation_aborted) {
      return;
    }
    DOODLE_LOG_ERROR("on_accept error: {}", ec.what());
  } else {
    entt::handle l_handle{*g_reg(), g_reg()->create()};
    l_handle.emplace<socket_logger>();
    l_handle.emplace<detail::http_route>(*route_ptr_);
    session::do_read{std::move(l_handle)}.run();
  }
  do_accept();
}
void working_machine::stop() {
  g_reg()->ctx().get<ue_task_manage>().cancel();
  g_reg()->ctx().get<computer_manage>().cancel();
  auto l_view = g_reg()->view<working_machine_session_data>();
  // close
  ranges::for_each(l_view, [](auto& in_session) { session::do_close{entt::handle{*g_reg(), in_session}}.run(); });

  acceptor_.cancel();
  acceptor_.close();
}

}  // namespace doodle::render_farm