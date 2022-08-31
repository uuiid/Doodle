//
// Created by TD on 2022/4/29.
//

#include "session_manager.h"
#include <json_rpc/core/server.h>
#include <json_rpc/core/session.h>


namespace doodle::json_rpc {
session_manager::session_manager() = default;

void session_manager::stop(const std::shared_ptr<session>& in_session) {
  session_list_.erase(in_session);
  in_session->stop();
}
void session_manager::stop_all() {
  for (auto&& i : session_list_) {
    i->stop();
  }
  session_list_.clear();
}
}  // namespace doodle::json_rpc
