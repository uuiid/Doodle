//
// Created by TD on 2024/2/27.
//

#include "websocket_route.h"
#include <doodle_lib/core/http/http_websocket_data.h>
namespace doodle::http {
websocket_route::map_actin_type websocket_route::operator()(const std::string& in_name) const {
  auto l_iter = actions_.find(in_name);
  if (l_iter == actions_.end()) return not_function_;
  return l_iter->second;
}

websocket_route& websocket_route::reg(const std::string& in_name, const map_actin_type& in_function) {
  actions_[in_name] = in_function;
  return *this;
}

void websocket_route::not_function(const http_websocket_data_ptr&  in_data, const nlohmann::json& in_json) {
  in_data->logger_->log(log_loc(), level::err, "websocket not found {}", in_json["type"].get<std::string>());
}
}  // namespace doodle::http
