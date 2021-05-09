#pragma once

#include <DoodleLib/DoodleLib_fwd.h>

DOODLE_NAMESPACE_S

class ScreenshotAction {
 public:
  ScreenshotAction();

  void screenShot(const FSys::path &save_path);

 private:
  bool p_isDrawing_b;

  std::string p_save_path;
};
DOODLE_NAMESPACE_E
