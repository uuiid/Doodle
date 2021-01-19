#pragma once

#include <doodle_global.h>

#include <src/ScreenshotWidght/ScreenshotWidght.h>
DOODLE_NAMESPACE_S
class AssScreenShotWidght : public ScreenshotWidght {
  Q_OBJECT
 public:
  AssScreenShotWidght(QWidget *parent = nullptr);

  void setIndexInfo(const assClassPtr &ass_class_ptr);

 private:
};

DOODLE_NAMESPACE_E