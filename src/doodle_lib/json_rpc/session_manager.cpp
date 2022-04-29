//
// Created by TD on 2022/4/29.
//

#include "session_manager.h"
#include <server/server.h>

session_manager::session_manager() = default;

void session_manager::start(const std::shared_ptr<session>& in_session) {
  session_list_.insert(in_session);
  in_session->start();
}
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
