//
// Created by td_main on 2023/9/14.
//
#include "url_webscoket.h"

#include <doodle_core/lib_warp/boost_fmt_asio.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>

#include "core/functional_registration_manager.h"
#include "core/websocket.h"
#include <doodle_server/render_farm/detail/computer.h>
#include <doodle_server/render_farm/work.h>

namespace doodle::render_farm {
namespace detail {

void reg_work_websocket::operator()() {
  auto& l_fun_reg = g_ctx().emplace<web_socket::functional_registration_manager>();
  l_fun_reg.register_function("run.ue.render.task", [](const entt::handle& in_handle, const nlohmann::json& in_json) {
    boost::system::error_code ec{};
    if (g_ctx().contains<render_farm::work_ptr>()) {
      ec = g_ctx().get<render_farm::work_ptr>()->run_job(in_handle, in_json);
    } else {
      log_error(in_handle.get<socket_logger>().logger_, fmt::format("没有在上下文中找到工作 {}", ec));
      BOOST_BEAST_ASSIGN_EC(ec, error_enum::not_find_work_class);
    }

    nlohmann::json l_json{};
    if (ec) {
      l_json["error"]["code"]    = ec.value();
      l_json["error"]["message"] = ec.message();
    } else {
      l_json["result"] = {};
    }

    return l_json;
  });
}

void reg_server_websocket::operator()() {
  struct run_reg_computer {
    nlohmann::json operator()(const entt::handle& in_handle, const nlohmann::json& in_json) {
      boost::system::error_code ec{};

      auto l_remote_ip = boost::beast::get_lowest_layer(in_handle.get<websocket_data>().stream_)
                             .socket()
                             .remote_endpoint()
                             .address()
                             .to_string();
      auto l_logger = in_handle.get<socket_logger>().logger_;
      log_info(l_logger, fmt::format("开始注册机器 {}", l_remote_ip));

      if (!in_handle.all_of<computer>()) {
        in_handle.emplace<computer>().set_name(l_remote_ip);
      }
      if (in_json.contains("status")) {
        in_handle.get<computer>().delay(in_json["status"].get<std::string>());
      } else {
        in_handle.get<computer>().delay();
      }

      nlohmann::json l_json{};
      if (ec) {
        l_json["error"]["code"]    = ec.value();
        l_json["error"]["message"] = ec.message();
      } else
        l_json["result"]["id"] = in_handle.entity();

      return l_json;
    }
  };

  auto& l_fun_reg = g_ctx().emplace<web_socket::functional_registration_manager>();
  l_fun_reg.register_function("run.reg.computer", run_reg_computer{});
}

}  // namespace detail
}  // namespace doodle::render_farm
