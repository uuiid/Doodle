//
// Created by TD on 2022/9/29.
//

#pragma once
#include <doodle_core/core/app_base.h>
#include <doodle_core/core/app_facet.h>
#include <doodle_core/doodle_core.h>

#include <doodle_app/app/authorization.h>
#include <doodle_app/app/program_options.h>
#include <doodle_app/doodle_app_fwd.h>
namespace doodle {
namespace details::app_command_base {
void run_facet(const app_base::app_facet_map& in_map);
}
/**
 * @brief 基本的命令行类
 *
 */
class DOODLE_APP_API app_command_base : public app_base {
 protected:
  bool chick_authorization() {
    DOODLE_LOG_INFO("开始检查授权");
    return authorization{}.is_expire();
  };

 public:
  app_command_base() : app_base() { program_options::emplace(); };
  virtual ~app_command_base() { program_options::reset(); }

  static app_command_base& Get() { return *(dynamic_cast<app_command_base*>(self)); }

 protected:
  virtual void post_constructor() override { details::app_command_base::run_facet(facet_list); };
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
