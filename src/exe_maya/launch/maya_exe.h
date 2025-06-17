//
// Created by TD on 2023/12/28.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <argh.h>
namespace doodle {

class maya_exe_main : public app_base {
 public:
  using app_base::app_base;

  bool init() override;
};

}  // namespace doodle