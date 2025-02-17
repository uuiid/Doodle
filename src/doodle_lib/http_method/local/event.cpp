//
// Created by TD on 25-1-23.
//

#include "event.h"

#include <doodle_lib/core/engine_io.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/socket_io/socket_io_core.h>

namespace doodle::http::local {

void local_event_reg(http_route& in_route) {
  auto l_sid_ctx = std::make_shared<socket_io::sid_ctx>();
  l_sid_ctx->on_connect([](const std::shared_ptr<socket_io::socket_io_core>& in_core) {
    in_core->emit("auth", in_core->auth_);
    // in_core->on_message(
    //     "message", socket_io::socket_io_core::slot_type{[l_s = std::weak_ptr{in_core}](const nlohmann::json& in_data)
    //     {
    //                  l_s.lock()->emit("message-back", in_data);
    //                }}.track_foreign(in_core)
    // );
    in_core->on_message(
        "message",
        socket_io::socket_io_core::slot_type{
            [l_s = std::weak_ptr{in_core}](const std::variant<nlohmann::json, std::vector<std::string>>& in_data) {
              if (std::holds_alternative<nlohmann::json>(in_data))
                l_s.lock()->emit("message-back", std::get<nlohmann::json>(in_data));
              else
                l_s.lock()->emit("message-back", std::get<std::vector<std::string>>(in_data));
            }
        }.track_foreign(in_core)
    );

    in_core->on_message(
        "message-with-ack",
        socket_io::socket_io_core::slot_type{
            [l_s = std::weak_ptr{in_core}](const std::variant<nlohmann::json, std::vector<std::string>>& in_data) {
              if (std::holds_alternative<nlohmann::json>(in_data))
                l_s.lock()->ask(std::get<nlohmann::json>(in_data));
              else
                l_s.lock()->ask(std::get<std::vector<std::string>>(in_data));
            }
        }.track_foreign(in_core)
    );
  });
  l_sid_ctx->on("/custom")->on_connect([](const std::shared_ptr<socket_io::socket_io_core>& in_core) {
    in_core->emit("auth", in_core->auth_);
  });
  socket_io::create_socket_io(in_route, l_sid_ctx);
}
}  // namespace doodle::http::local