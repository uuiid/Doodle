//
// Created by TD on 2022/5/30.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>
namespace doodle::database_n {

class DOODLE_CORE_API select {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  select();

  virtual ~select();

  bool operator()(entt::registry& in_registry, const FSys::path& in_project_path, const sql_connection_ptr& in_connect);

  bool is_old(const FSys::path& in_project_path, const sql_connection_ptr& in_connect);

  void patch();
};

}  // namespace doodle::database_n
