#pragma once

#include <DoodleLib/doodle_lib_fwd.h>

namespace doodle {

class screenshot_action {
 public:
  screenshot_action();

  void screenShot(const FSys::path &save_path);

 private:
  bool p_isDrawing_b;

  std::string p_save_path;
};
}
