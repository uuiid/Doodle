//
// Created by td_main on 2023/4/26.
//

#pragma once
#include <doodle_core/core/file_sys.h>
namespace doodle {
namespace maya_plug {

class maya_lib_guard {
 public:
  explicit maya_lib_guard();
  ~maya_lib_guard();

 private:
};

}  // namespace maya_plug
}  // namespace doodle
