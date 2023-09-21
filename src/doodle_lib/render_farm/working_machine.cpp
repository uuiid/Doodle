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
  route_ptr_->reg<detail::get_root_type>();
  route_ptr_->reg<detail::render_job_type_post>();
  route_ptr_->reg<detail::computer_reg_type_post>();

  route_ptr_->reg<detail::get_log_type_get>();
  route_ptr_->reg<detail::get_err_type_get>();
  route_ptr_->reg<detail::render_job_type_get>();
  route_ptr_->reg<detail::repository_type_get>();
  route_ptr_->reg<detail::computer_reg_type_get>();

  route_ptr_->reg<detail::render_job_type_post>();
  route_ptr_->reg<detail::get_log_type_post>();
  route_ptr_->reg<detail::get_err_type_post>();

  route_ptr_->reg<detail::render_job_type_put>();

  route_ptr_->reg<detail::computer_reg_type_websocket>();

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
    entt::handle l_handle{*g_reg(), g_reg()->create()};
    l_handle.emplace<socket_logger>();
    l_handle.emplace<working_machine_session>(std::move(socket), route_ptr_).run();
  }
  do_accept();
}
void working_machine::stop() {
  g_reg()->ctx().get<ue_task_manage>().cancel();
  g_reg()->ctx().get<computer_manage>().cancel();
  auto l_view = g_reg()->view<working_machine_session>();
  // close
  ranges::for_each(l_view, [](auto& in_session) { g_reg()->get<working_machine_session>(in_session).do_close(); });

  acceptor_.cancel();
  acceptor_.close();
}


}  // namespace doodle::render_farm