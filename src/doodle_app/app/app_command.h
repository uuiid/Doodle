//
// Created by TD on 2022/9/29.
//

#pragma once
#include <doodle_core/core/app_base.h>
#include <doodle_core/core/app_facet.h>
#include <doodle_core/doodle_core.h>

#include <doodle_app/app/authorization.h>
#include <doodle_app/app/facet/gui_facet.h>
#include <doodle_app/app/program_options.h>
#include <doodle_app/doodle_app_fwd.h>
#include <doodle_app/gui/main_proc_handle.h>

namespace doodle {
namespace details::app_command_base {
void run_facet(const app_base::app_facet_map& in_map, app_base::app_facet_ptr& in_def_facet);
}
/**
 * @brief 基本的命令行类
 *  ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
 */
template <typename Facet_Defaute, typename... Facet_>
class app_command : public app_base {
 protected:
  bool chick_authorization() {
    DOODLE_LOG_INFO("开始检查授权");
    return authorization{}.is_expire();
  };

 public:
  app_command() : app_base() {
    program_options::emplace();
    run_facet = std::make_shared<Facet_Defaute>();
    add_facet(run_facet);
    (add_facet(std::make_shared<Facet_>()), ...);
  };
  virtual ~app_command() { program_options::reset(); }

  static app_command& Get() { return *(dynamic_cast<app_command*>(self)); }

 protected:
  virtual void post_constructor() override { details::app_command_base::run_facet(facet_list, run_facet); };
};


}  // namespace doodle
