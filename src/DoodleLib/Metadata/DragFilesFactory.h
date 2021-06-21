//
// Created by TD on 2021/6/17.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {
class DOODLELIB_API DragFilesFactory {
  std::vector<FSys::path> p_paths;
  std::vector<ActionPtr> p_action;

  void runChick();
 public:
  explicit DragFilesFactory(std::vector<FSys::path> in_paths);

  bool has_action();

  ActionPtr get_action();
  ActionPtr operator()();
};
}  // namespace doodle
