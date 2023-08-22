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
  //  do_wait();
  //  do_resolve();

  async_connect(
      boost::asio::any_io_executor{boost::asio::make_strand(g_io_context())},
      [](boost::system::error_code ec, socket_ptr in_ptr) {
        if (ec) {
          DOODLE_LOG_INFO("{}", ec.message());
          return;
        }
        DOODLE_LOG_INFO("连接成功服务器");
      }
  );
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
  do_hello();
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

void client::do_write() {
  boost::beast::async_write(
      ptr_->socket_->socket(), std::move(*ptr_->request_), boost::beast::bind_front_handler(&client::on_write, this)
  );
}

void client::on_write(boost::system::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec == boost::beast::errc::not_connected || ec == boost::beast::errc::connection_reset ||
      ec == boost::beast::errc::connection_refused || ec == boost::beast::errc::connection_aborted) {
    DOODLE_LOG_INFO("失去连接, 开始重新连接");
    do_resolve();
    return;
  }
  do_read();
}

void client::do_read() {
  ptr_->buffer_.clear();
  ptr_->response_ = {};
  boost::beast::http::async_read(
      ptr_->socket_->socket(), ptr_->buffer_, ptr_->response_, boost::beast::bind_front_handler(&client::on_read, this)
  );
}
void client::on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec) {
    DOODLE_LOG_INFO("{}", ec.message());
    return;
  }
  DOODLE_LOG_INFO("{}", ptr_->response_.body());
}

void client::rest_run() {
  ptr_->connect_count_ = 0;
  run();
}
void client::list_task() {
  boost::beast::http::request<render_farm::detail::basic_json_body> l_request{
      boost::beast::http::verb::get, "/v1/render_farm/render_job", 11};
  l_request.keep_alive(true);
  l_request.set(boost::beast::http::field::host, fmt::format("{}:50021", ptr_->server_ip_));
  l_request.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_request.set(boost::beast::http::field::content_type, "application/json");
  l_request.set(boost::beast::http::field::accept, "application/json");
}
void client::do_hello() {
  boost::beast::http::request<boost::beast::http::empty_body> l_request{
      boost::beast::http::verb::get, "/v1/render_farm", 11};
  l_request.keep_alive(true);
  l_request.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  ptr_->request_ = std::make_shared<message_generator_type>(std::move(l_request));
}
}  // namespace doodle