//
// Created by TD on 2021/6/17.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {
class DOODLELIB_API DragFilesFactory {
  std::vector<FSys::path> p_paths;
  std::vector<action_ptr> p_action;

  void runChick();
 public:
  explicit DragFilesFactory(std::vector<FSys::path> in_paths);

  bool has_action();

  std::vector<action_ptr> get_action();
  std::vector<action_ptr> operator()();
};
}  // namespace doodle
