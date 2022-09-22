//
// Created by TD on 2022/5/30.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/gui_template/gui_process.h>
namespace doodle {
namespace database_n {

class DOODLE_CORE_API select : public process_handy_tools {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  select();

  ~select() override;
  void init();
  void aborted();

  void operator()(
      entt::registry& in_registry,
      const FSys::path& in_project_path,
      conn_ptr& in_connect
  );
};

}  // namespace database_n
}  // namespace doodle
