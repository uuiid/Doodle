//
// Created by td_main on 2023/8/10.
//

#include "computer.h"

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/bind_front_handler.h>
#include <doodle_lib/render_farm/detail/basic_json_body.h>
#include <doodle_lib/render_farm/detail/ue4_task.h>

#include <boost/beast.hpp>

#include <magic_enum.hpp>
namespace doodle {
namespace render_farm {
namespace {

class send_to_render {
 public:
  explicit send_to_render(std::string in_ip, const entt::handle& in_task)
      : socket_{g_io_context()}, ip_{std::move(in_ip)}, task_ptr_{in_task.get<ue4_task_ptr>()} {}

  void run() {
    boost::asio::async_connect(
        socket_, boost::asio::ip::tcp::resolver(g_io_context()).resolve(ip_, "50021"),
        bind_reg_handler(&send_to_render::on_connect, g_reg(), this)
    );
  };

 private:
  // on_connect
  void on_connect(boost::system::error_code ec, boost::asio::ip::tcp::endpoint endpoint) {
    if (ec) {
      DOODLE_LOG_INFO("{}", ec.message());
      return;
    }
    boost::beast::http::request<detail::basic_json_body> l_request{
        boost::beast::http::verb::post, "/v1/render_farm/run_job", 11};
    nlohmann::json l_json{};
    l_json["id"]     = entt::to_entity(*g_reg(), *this);
    l_json["arg"]    = task_ptr_->arg();
    l_request.body() = l_json;
    l_request.keep_alive(false);
    l_request.prepare_payload();
    boost::beast::http::async_write(socket_, l_request, bind_reg_handler(&send_to_render::on_write, g_reg(), this));
  }

  // on write
  void on_write(boost::system::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
      DOODLE_LOG_INFO("{}", ec.message());
      return;
    }

    boost::beast::http::async_read(
        socket_, buffer_, response, bind_reg_handler(&send_to_render::on_read, g_reg(), this)
    );
  }

  // on read
  void on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
      DOODLE_LOG_INFO("{}", ec.message());
      return;
    }
    DOODLE_LOG_INFO("{}", response.body().dump());
  }

  boost::beast::flat_buffer buffer_;
  boost::beast::http::response<detail::basic_json_body> response;
  boost::asio::ip::tcp::socket socket_;
  std::string ip_;
  ue4_task_ptr task_ptr_;
};
}  // namespace

void computer::delay(computer_status in_status) {
  if (chrono::sys_seconds::clock::now() - last_time_ < 0.5s) {
    return;
  }
  if (status_ == computer_status::busy) return;

  status_ = in_status;
}
void computer::delay(const std::string& in_str) {
  auto l_status = magic_enum::enum_cast<computer_status>(in_str);
  delay(l_status.value_or(computer_status::idle));
}

void computer::run_task(const entt::handle& in_handle) {
  status_ = computer_status::busy;

  make_handle(this).emplace<send_to_render>(name_, in_handle).run();
}
}  // namespace render_farm
}  // namespace doodle