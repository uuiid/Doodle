//
// Created by TD on 2021/6/17.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {
class DOODLELIB_API DragFilesFactory {
 public:
  DragFilesFactory();

  ActionPtr get_action();
  ActionPtr operator()();
};
}  // namespace doodle
