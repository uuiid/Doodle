//
// Created by TD on 25-2-17.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::socket_io {
class sid_ctx;
void broadcast(
    const std::string& in_event, const nlohmann::json& in_data, const std::string& in_namespace = {"/events"},
    const std::shared_ptr<sid_ctx>& in_ctx = nullptr
);

template <typename To_Jsonable>
concept Jsonable = requires(const To_Jsonable& t) {
  { nlohmann::json{} = t };
};
template <Jsonable To_Jsonable>
void broadcast(
    const std::string& in_event, const To_Jsonable& in_data, const std::string& in_namespace = {"/events"},
    const std::shared_ptr<sid_ctx>& in_ctx = nullptr
) {
  broadcast(in_event, nlohmann::json{} = in_data, in_namespace, in_ctx);
}
}  // namespace doodle::socket_io