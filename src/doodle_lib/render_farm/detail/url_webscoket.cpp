//
// Created by td_main on 2023/9/14.
//
#include "url_webscoket.h"

#include <doodle_core/lib_warp/boost_fmt_asio.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <doodle_lib/render_farm/detail/computer.h>
#include <doodle_lib/render_farm/functional_registration_manager.h>
#include <doodle_lib/render_farm/websocket.h>
#include <doodle_lib/render_farm/work.h>

namespace doodle::render_farm {
namespace detail {

void computer_reg_type_websocket::operator()(
    const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap
) const {
  auto& l_session   = in_handle.get<working_machine_session>();
  auto l_parser_ptr = std::make_shared<boost::beast::http::request_parser<boost::beast::http::string_body>>(
      std::move(l_session.request_parser())
  );
  boost::beast::http::async_read(
      l_session.stream(), l_session.buffer(), *l_parser_ptr,
      [in_handle, l_parser_ptr](boost::system::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        auto& l_session = in_handle.get<working_machine_session>();
        if (ec == boost::beast::http::error::end_of_stream) {
          return l_session.do_close();
        }
        if (ec) {
          log_error(l_session.logger(), fmt::format("on_read error: {} ", ec));
          l_session.send_error_code(ec);
          return;
        }
        in_handle.emplace<render_farm::websocket_data>(l_session.stream().release_socket());
        std::make_shared<render_farm::websocket>(in_handle)->run(l_parser_ptr->release());
      }
  );
}

void reg_work_websocket::operator()() {
  auto& l_fun_reg = g_ctx().emplace<render_farm::functional_registration_manager>();
  l_fun_reg.register_function("run.ue.render.task", [](const entt::handle& in_handle, const nlohmann::json& in_json) {
    boost::system::error_code ec{};
    if (g_ctx().contains<render_farm::work_ptr>()) {
      ec = g_ctx().get<render_farm::work_ptr>()->run_job(in_handle, in_json);
    } else {
      log_error(in_handle.get<websocket_data>().logger_, fmt::format("没有在上下文中找到工作 {}", ec));
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
                             .remote_endpoint()
                             .address()
                             .to_string();
      auto l_logger = in_handle.get<websocket_data>().logger_;
      log_info(l_logger, fmt::format("开始注册机器 {}", l_remote_ip));

      if (!in_handle || !in_handle.all_of<computer>()) {
        in_handle.emplace<computer>().set_name(l_remote_ip);
      }
      if (in_json.contains("status")) {
        in_handle.get<computer>().delay(in_json["status"].get<std::string>();
      } else {
        in_handle.get<computer>().delay();
      }

      nlohmann::json l_json{};
      if (ec) {
        l_json["error"]["code"]    = ec.value();
        l_json["error"]["message"] = ec.message();
      } else
        l_json["result"] = in_handle.entity();

      return l_json;
    }
  };

  auto& l_fun_reg = g_ctx().emplace<render_farm::functional_registration_manager>();
  l_fun_reg.register_function("run.reg.computer", run_reg_computer{});
}

}  // namespace detail
}  // namespace doodle::render_farm
