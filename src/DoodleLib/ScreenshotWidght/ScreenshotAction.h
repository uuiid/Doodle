#pragma once

#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {

class ScreenshotAction {
 public:
  ScreenshotAction();

  void screenShot(const FSys::path &save_path);

 private:
  bool p_isDrawing_b;

  std::string p_save_path;
};
}
