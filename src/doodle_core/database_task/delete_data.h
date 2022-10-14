//
// Created by TD on 2022/5/30.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/gui_template/gui_process.h>
namespace doodle::database_n {

class DOODLE_CORE_API delete_data {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  explicit delete_data(const std::vector<entt::entity>& in_data);
  delete_data();

  virtual ~delete_data();

  void operator()(entt::registry& in_registry, const std::vector<entt::entity>& in_update_data, conn_ptr& in_connect);
};

}  // namespace doodle::database_n
