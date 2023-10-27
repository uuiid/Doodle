//
// Created by td_main on 2023/8/3.
//

#include "http_listener.h"

#include "doodle_app/app/app_command.h"

#include "boost/asio.hpp"
#include "boost/beast.hpp"

#include "http_session.h"
#include "render_farm/detail/computer_manage.h"
#include "render_farm/detail/ue_task_manage.h"
#include "render_farm/detail/url_route_base.h"
#include "render_farm/detail/url_route_get.h"
#include "render_farm/detail/url_route_post.h"
#include "render_farm/detail/url_route_put.h"
#include "render_farm/detail/url_webscoket.h"
#include "render_farm/render_farm_fwd.h"
namespace doodle {

void http_listener::cancellation_signals::emit(boost::asio::cancellation_type ct) {
  std::lock_guard<std::mutex> _(mtx);

  for (auto& sig : sigs) sig.emit(ct);
}

boost::asio::cancellation_slot http_listener::cancellation_signals::slot() {
  std::lock_guard<std::mutex> _(mtx);

  auto itr = std::find_if(sigs.begin(), sigs.end(), [](boost::asio::cancellation_signal& sig) {
    return !sig.slot().has_handler();
  });

  if (itr != sigs.end())
    return itr->slot();
  else
    return sigs.emplace_back().slot();
}

void http_listener::run() {
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
  acceptor_ptr_ = std::make_shared<acceptor_type>(g_io_context(), end_point_);
  do_accept();
  signal_set_.async_wait([&](boost::system::error_code ec, int signal) {
    if (ec) {
      DOODLE_LOG_ERROR("signal_set_ error: {}", ec.message());
      return;
    }
    DOODLE_LOG_INFO("signal_set_ signal: {}", signal);
    cancellation_signals_.emit();
    this->stop();
    //    app_base::Get().stop_app();
  });
}
void http_listener::do_accept() {
  acceptor_ptr_->async_accept(
      boost::asio::make_strand(g_io_context()),
      boost::asio::bind_cancellation_slot(
          cancellation_signals_.slot(), boost::beast::bind_front_handler(&http_listener::on_accept, this)
      )
  );
}
void http_listener::on_accept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
  if (ec) {
    if (ec == boost::asio::error::operation_aborted) {
      return;
    }
    DOODLE_LOG_ERROR("on_accept error: {}", ec.what());
  } else {
    entt::handle l_handle{*g_reg(), g_reg()->create()};
    l_handle.emplace<socket_logger>();
    l_handle.emplace<detail::http_route>(*route_ptr_);
    l_handle.emplace<http_session_data>(std::move(socket));
    session::do_read{std::move(l_handle)}.run();
  }
}
void http_listener::stop() {
  g_reg()->ctx().get<ue_task_manage>().cancel();
  g_reg()->ctx().get<computer_manage>().cancel();
  auto l_view = g_reg()->view<http_session_data>();
  // close
  ranges::for_each(l_view, [](auto& in_session) { session::do_close{entt::handle{*g_reg(), in_session}}.run(); });
}

}  // namespace doodle::render_farm