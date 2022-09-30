//
// Created by TD on 2022/9/29.
//

#pragma once
#include <doodle_app/doodle_app_fwd.h>
#include <doodle_core/doodle_core.h>
#include <doodle_core/core/app_base.h>
#include <doodle_app/app/program_options.h>
namespace doodle {

/**
 * @brief 基本的命令行类
 *
 */
class DOODLE_APP_API app_command_base : public app_base {
 protected:
  cmd_string_type cmd_str;


  virtual bool chick_authorization();
  virtual void load_facet();

  std::optional<FSys::path> find_authorization_file() const;
  bool chick_build_time() const;

 public:
  explicit app_command_base(const app_base::in_app_args& in_instance);

  static app_command_base& Get();
  //    std::vector<std::string> l_str{argv, argv + argc};

 protected:
  virtual void post_constructor() override;
};

}  // namespace doodle
