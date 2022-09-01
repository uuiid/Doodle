//
// Created by TD on 2022/5/30.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/gui_template/gui_process.h>
namespace doodle {
namespace database_n {

class DOODLE_CORE_EXPORT select : public process_handy_tools {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  class DOODLE_CORE_EXPORT arg {
   public:
    /**
     * @brief 这个是指向数据库的绝对路径
     */
    FSys::path project_path;
    bool only_ctx;
  };
  explicit select(const arg& in_arg);
  select();

  ~select() override;
  void init();
  void aborted();

  void operator()(
      entt::registry& in_registry,
      const FSys::path& in_project_path
  );

 private:
  void th_run();
};

}  // namespace database_n
}  // namespace doodle
