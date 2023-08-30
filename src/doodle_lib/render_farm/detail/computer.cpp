//
// Created by td_main on 2023/8/10.
//

#include "computer.h"

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/bind_front_handler.h>
#include <doodle_lib/render_farm/client_core.h>
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
  std::shared_ptr<doodle::detail::client_core> client_core_ptr_{};
  entt::handle task_handle_;

 public:
  explicit send_to_render(std::string in_ip, entt::handle in_task)
      : client_core_ptr_(), task_handle_{std::move(in_task)} {
    client_core_ptr_ = std::make_shared<doodle::detail::client_core>(in_ip);
  }

  void run() {
    using request_type_1  = boost::beast::http::request<detail::basic_json_body>;
    using response_type_1 = boost::beast::http::response<detail::basic_json_body>;
    request_type_1 l_request{boost::beast::http::verb::post, "/v1/render_farm/run_job", 11};
    nlohmann::json l_json{};
    l_json["id"]     = task_handle_.entity();
    l_json["arg"]    = task_handle_.get<detail::ue4_task>().arg();
    l_request.body() = l_json;
    l_request.keep_alive(false);
    l_request.prepare_payload();
    client_core_ptr_->async_read<boost::beast::http::response<detail::basic_json_body>>(
        boost::asio::make_strand(g_io_context()), l_request,
        [this](auto&& PH1, const response_type_1& PH2) {
          if (PH1) {
            DOODLE_LOG_ERROR("{}", PH1.message());
            return;
          }
          if (PH2.result() == boost::beast::http::status::ok) {
            DOODLE_LOG_INFO("成功派发任务");
          } else {
            DOODLE_LOG_INFO("派发任务失败");
          }
        }
    );
  };
};
}  // namespace

void computer::delay(computer_status in_status) {
  if (chrono::sys_seconds::clock::now() - last_time_ < 0.5s) {
    return;
  }

  status_ = in_status;
}
void computer::delay(const std::string& in_str) {
  auto l_status = magic_enum::enum_cast<computer_status>(in_str);
  delay(l_status.value_or(computer_status::idle));
}

void computer::run_task(const entt::handle& in_handle) {
  status_            = computer_status::busy;
  auto l_self_handle = make_handle(this);
  l_self_handle.get_or_emplace<send_to_render>(name_, in_handle).run();
}
}  // namespace render_farm
}  // namespace doodle