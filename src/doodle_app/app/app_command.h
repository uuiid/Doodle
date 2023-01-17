//
// Created by TD on 2022/9/29.
//

#pragma once
#include <doodle_core/core/app_base.h>
#include <doodle_core/doodle_core.h>

#include <doodle_app/doodle_app_fwd.h>

namespace doodle {

/**
 * @brief 基本的命令行类
 *
 */
class DOODLE_APP_API app_command_base : public app_base {
 protected:
  std::vector<std::string> cmd_str;

  virtual bool chick_authorization();

  std::optional<FSys::path> find_authorization_file() const;
  bool chick_build_time() const;

 public:
  app_command_base();

  static app_command_base& Get();
  //    std::vector<std::string> l_str{argv, argv + argc};

 protected:
  virtual void post_constructor() override;
};

/**
 * @brief
 *
 */
class DOODLE_APP_API doodle_main_app : public app_command_base {
 public:
  class in_gui_arg : public app_base::in_app_args {
   public:
    std::int32_t show_enum;
    win::wnd_handle in_parent;
  };
  explicit doodle_main_app();

  ~doodle_main_app() override;

  static doodle_main_app& Get();

 protected:
  virtual bool chick_authorization() override;
};
}  // namespace doodle
