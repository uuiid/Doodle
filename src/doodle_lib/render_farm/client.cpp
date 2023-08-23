//
// Created by td_main on 2023/8/18.
//

#include "client.h"

#include <doodle_lib/core/bind_front_handler.h>

#include <boost/asio.hpp>
namespace doodle {

client::queue_action_guard::~queue_action_guard() {
  ptr_->queue_running_ = false;
  ptr_->queue_.pop();
  if (!ptr_->queue_.empty()) {
    ptr_->queue_.front()();
  }
}

void client::do_wait() {
  ptr_->timer_->expires_after(5s);
  //  ptr_->timer_->async_wait(boost::beast::bind_front_handler(&client::on_connect_timeout, this));
}

void client::make_ptr() {
  auto l_s        = boost::asio::make_strand(g_io_context());
  ptr_->socket_   = std::make_shared<socket_t>(l_s);
  ptr_->timer_    = std::make_shared<timer_t>(l_s);
  ptr_->resolver_ = std::make_shared<resolver_t>(l_s);
  //  ptr_->signal_set_ = std::make_shared<signal_set>(g_io_context(), SIGINT, SIGTERM);
}

void client::run() {
  for (int l = 0; l < 100; ++l) {
    async_connect(boost::asio::make_strand(g_io_context()), [](boost::system::error_code ec, std::string in_string) {
      if (ec) {
        DOODLE_LOG_INFO("{}", ec.message());
        return;
      }
      DOODLE_LOG_INFO("连接成功服务器 {}", in_string);
    });
  }
}
boost::beast::http::message_generator client::task_list_t::operator()() {
  boost::beast::http::request<render_farm::detail::basic_json_body> l_request{
      boost::beast::http::verb::get, "/v1/render_farm/render_job", 11};
  l_request.keep_alive(true);
  l_request.set(boost::beast::http::field::host, fmt::format("{}:50021", ptr_->server_ip()));
  l_request.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_request.set(boost::beast::http::field::content_type, "application/json");
  l_request.set(boost::beast::http::field::accept, "application/json");
  return {std::move(l_request)};
}
client::task_list_t::result_type client::task_list_t::operator()(const client::response_type& in_response) {
  return in_response.body().get<std::vector<task_t>>();
}

boost::beast::http::message_generator client::computer_list_t::operator()() {
  boost::beast::http::request<render_farm::detail::basic_json_body> l_request{
      boost::beast::http::verb::get, "/v1/render_farm/computer", 11};
  l_request.keep_alive(true);
  l_request.set(boost::beast::http::field::host, fmt::format("{}:50021", ptr_->server_ip()));
  l_request.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  l_request.set(boost::beast::http::field::content_type, "application/json");
  l_request.set(boost::beast::http::field::accept, "application/json");
  return {std::move(l_request)};
}
client::computer_list_t::result_type client::computer_list_t::operator()(const client::response_type& in_response) {
  return in_response.body().get<std::vector<computer>>();
}

}  // namespace doodle