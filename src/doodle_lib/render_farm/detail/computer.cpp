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
class send_to_render;
using send_to_render_ptr = std::shared_ptr<send_to_render>;
class send_to_render {
 private:
  struct data_type {
    std::string ip_;
    entt::handle task_handle_;
    boost::asio::ip::tcp::socket socket_{g_io_context()};
    boost::beast::flat_buffer buffer_;
    boost::beast::http::response<detail::basic_json_body> response;
    boost::beast::http::request<detail::basic_json_body> request_;
  };
  std::shared_ptr<data_type> ptr_;

 public:
  explicit send_to_render(std::string in_ip, entt::handle in_task) : ptr_(std::make_shared<data_type>()) {
    ptr_->ip_          = std::move(in_ip);
    ptr_->task_handle_ = std::move(in_task);
  }

  void run() {
    boost::asio::async_connect(
        ptr_->socket_, boost::asio::ip::tcp::resolver(g_io_context()).resolve(ptr_->ip_, "50021"),
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
    ptr_->request_ = boost::beast::http::request<detail::basic_json_body>{
        boost::beast::http::verb::post, "/v1/render_farm/run_job", 11};
    nlohmann::json l_json{};
    l_json["id"]          = entt::to_entity(*g_reg(), *this);
    l_json["arg"]         = ptr_->task_handle_.get<detail::ue4_task>().arg();
    ptr_->request_.body() = l_json;
    ptr_->request_.keep_alive(false);
    ptr_->request_.prepare_payload();
    boost::beast::http::async_write(
        ptr_->socket_, ptr_->request_, bind_reg_handler(&send_to_render::on_write, g_reg(), this)
    );
  }

  // on write
  void on_write(boost::system::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
      DOODLE_LOG_INFO("{}", ec.message());
      return;
    }

    boost::beast::http::async_read(
        ptr_->socket_, ptr_->buffer_, ptr_->response, bind_reg_handler(&send_to_render::on_read, g_reg(), this)
    );
  }

  // on read
  void on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
      DOODLE_LOG_INFO("{}", ec.message());
      return;
    }
    DOODLE_LOG_INFO("{}", ptr_->response.body().dump());
  }
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
  status_            = computer_status::busy;
  auto l_self_handle = make_handle(this);
  l_self_handle.emplace<send_to_render>(name_, in_handle).run();
}
}  // namespace render_farm
}  // namespace doodle