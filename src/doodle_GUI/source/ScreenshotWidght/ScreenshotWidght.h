#pragma once

#include <doodle_GUI/doodle_global.h>

DOODLE_NAMESPACE_S

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

DOODLE_NAMESPACE_E