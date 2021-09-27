#pragma once

#include <DoodleLib/DoodleLib_fwd.h>

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
