//
// Created by TD on 2024/2/27.
//

#include "websocket_route.h"
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/http_session_data.h>

namespace doodle::http {
websocket_route::call_fun_type websocket_route::operator()(const std::string& in_name) const {
  auto l_iter = actions_.find(in_name);
  if (l_iter == actions_.end()) return not_function_;
  return l_iter->second;
}

websocket_route& websocket_route::reg(const std::string& in_name, const call_fun_type& in_function) {
  actions_[in_name] = in_function;
  return *this;
}

boost::asio::awaitable<std::string> websocket_route::not_function(http_websocket_data_ptr in_data) {
  in_data->logger_->log(log_loc(), level::err, "websocket not found {}", in_data->body_["type"].get<std::string>());
  co_return std::string{};
}
} // namespace doodle::http