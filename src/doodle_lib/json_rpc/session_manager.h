//
// Created by TD on 2022/4/29.
//
#pragma once
#include <vector>
#include <set>
#include <memory>
class session;
class session_manager {
 private:
  std::set<std::shared_ptr<session>> session_list_;

 public:
  session_manager();

  void start(const std::shared_ptr<session>& in_session);
  void stop(const std::shared_ptr<session>& in_session);
  void stop_all();
};
