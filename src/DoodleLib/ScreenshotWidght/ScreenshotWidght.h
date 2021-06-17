#pragma once

#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {

class ScreenshotWidght {
 public:
  ScreenshotWidght();

  virtual void createScreenshot();
  void showImage();
  void clearImage();

  void disableButten(bool disable);

 private:
 protected:
};

}
