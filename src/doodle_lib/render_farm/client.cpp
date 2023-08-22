//
// Created by td_main on 2023/8/18.
//

#include "client.h"

#include <doodle_lib/core/bind_front_handler.h>

#include <boost/asio.hpp>
namespace doodle {
void client::do_wait() {
  ptr_->timer_->expires_after(5s);
  ptr_->timer_->async_wait(boost::beast::bind_front_handler(&client::on_connect_timeout, this));
}

void client::make_ptr() {
  auto l_s        = boost::asio::make_strand(g_io_context());
  ptr_->socket_   = std::make_shared<socket>(l_s);
  ptr_->timer_    = std::make_shared<timer>(l_s);
  ptr_->resolver_ = std::make_shared<resolver>(l_s);
  //  ptr_->signal_set_ = std::make_shared<signal_set>(g_io_context(), SIGINT, SIGTERM);
}

void client::run() {
  if (ptr_->connect_count_ > 3) {
    DOODLE_LOG_INFO("连接服务器失败");
    return;
  }

  //  boost::asio::bind_cancellation_slot();
  ++ptr_->connect_count_;
  do_wait();
  do_resolve();
}

void client::do_resolve() {
  ptr_->resolver_->async_resolve(
      ptr_->server_ip_, "50021", boost::beast::bind_front_handler(&client::on_resolve, this)
  );
}
void client::on_resolve(boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type results) {
  if (ec) {
    DOODLE_LOG_INFO("{}", ec.message());
    return;
  }
  boost::asio::async_connect(
      ptr_->socket_->socket(), results, boost::beast::bind_front_handler(&client::on_connect, this)
  );
}
void client::on_connect(boost::system::error_code ec, boost::asio::ip::tcp::endpoint endpoint) {
  boost::ignore_unused(endpoint);
  if (ec) {
    DOODLE_LOG_INFO("{}", ec.message());
    return;
  }
  DOODLE_LOG_INFO("连接成功服务器");
}

void client::on_connect_timeout(boost::system::error_code ec) {
  if (ec) {
    DOODLE_LOG_INFO("{}", ec.message());
    return;
  }
  //  ptr_->socket_->socket().cancel();
  DOODLE_LOG_INFO("连接服务器超时");
  //  run();
}
void client::rest_run() {
  ptr_->connect_count_ = 0;
  run();
}
void client::list_task() {
  if (!ptr_->socket_.is_open()) {
    DOODLE_LOG_INFO("socket is not open");
    return;
  }

  ptr_->request_ = boost::beast::http::request<render_farm::detail::basic_json_body>{
      boost::beast::http::verb::get, "/v1/render_farm/render_job", 11};
  ptr_->request_.keep_alive(true);
  ptr_->request_.set(boost::beast::http::field::host, fmt::format("{}:50021", ptr_->server_ip_));
  ptr_->request_.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  ptr_->request_.set(boost::beast::http::field::content_type, "application/json");
  ptr_->request_.set(boost::beast::http::field::accept, "application/json");
}
}  // namespace doodle