//
// Created by TD on 2022/5/30.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>
namespace doodle::database_n {

class DOODLE_CORE_API update_data {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  explicit update_data(const std::vector<entt::entity>& in_data);
  update_data();

  virtual ~update_data();

  void operator()(entt::registry& in_registry, const std::vector<entt::entity>& in_update_data, conn_ptr& in_connect);
};

}  // namespace doodle::database_n
