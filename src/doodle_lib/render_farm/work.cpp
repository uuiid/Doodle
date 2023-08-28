//
// Created by td_main on 2023/8/21.
//

#include "work.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <boost/beast.hpp>
namespace doodle {
namespace render_farm {
void work::on_wait(boost::system::error_code ec) {
  if (ec == boost::asio::error::operation_aborted) {
    return;
  }
  if (ec) {
    DOODLE_LOG_INFO("{}", ec);
    return;
  }
  run();
}
void work::do_wait() {
  DOODLE_LOG_INFO("开始等待下一次心跳");
  ptr_->timer_->expires_after(doodle::chrono::seconds{2});
  ptr_->timer_->async_wait(std::bind(&work::on_wait, this, std::placeholders::_1));
}

void work::make_ptr() {
  auto l_s          = boost::asio::make_strand(g_io_context());
  ptr_->timer_      = std::make_shared<timer>(l_s);
  ptr_->signal_set_ = std::make_shared<signal_set>(l_s, SIGINT, SIGTERM);
  ptr_->signal_set_->async_wait([&](boost::system::error_code ec, int signal) {
    if (ec) {
      DOODLE_LOG_ERROR("signal_set_ error: {}", ec);
      return;
    }
    DOODLE_LOG_INFO("signal_set_ signal: {}", signal);
    this->do_close();
  });
}

void work::run() {
  ptr_->core_ptr_->async_main(
      boost::asio::make_strand(g_io_context()),
      [this](auto&& PH1, auto&& PH2) {
        DOODLE_LOG_INFO("{}", PH2);
        do_wait();
      },
      reg_action{}
  );
}

void work::send_server_state(const entt::handle& in_handle) {}

void work::do_close() { ptr_->core_ptr_->cancel(); }
boost::beast::http::message_generator work::reg_action::operator()() {
  request_type l_request{boost::beast::http::verb::post, "/v1/render_farm/computer", 11};
  l_request.set(boost::beast::http::field::content_type, "application/json");
  l_request.set(boost::beast::http::field::accept, "application/json");
  nlohmann::json l_json;
  l_json["name"]   = boost::asio::ip::host_name();
  l_request.body() = l_json.dump();
  l_request.prepare_payload();
  return l_request;
}
work::reg_action::result_type work::reg_action::operator()(const work::reg_action::response_type& in_response) {
  return in_response.body();
}
}  // namespace render_farm
}  // namespace doodle