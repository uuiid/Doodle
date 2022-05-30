//
// Created by TD on 2022/1/18.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_core/core/app_base.h>

namespace doodle {

/**
 * @brief 基本的命令行类
 *
 */
class DOODLELIB_API app_command_base : public app_base {
 protected:
  void load_back_end() override;

 public:
  using app_base::app_base;

  static app_command_base& Get();
};

}  // namespace doodle
