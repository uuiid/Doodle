#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {

class screenshot_widght {
 public:
  screenshot_widght();

  virtual void createScreenshot();
  void showImage();
  void clearImage();

  void disableButten(bool disable);

 private:
 protected:
};

}
