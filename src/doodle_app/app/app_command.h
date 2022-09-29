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

  void load_back_end() override;
  virtual bool chick_authorization();

 public:
  explicit app_command_base(const app_base::in_app_args& in_instance);

  static app_command_base& Get();
  //    std::vector<std::string> l_str{argv, argv + argc};

  bool chick_authorization(const FSys::path& in_path);
  program_options_ptr options_;

 protected:
  virtual void post_constructor() override;
};

}  // namespace doodle
