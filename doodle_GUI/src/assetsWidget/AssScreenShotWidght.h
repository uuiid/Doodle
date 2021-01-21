#pragma once

#include <core_global.h>
#include <doodle_global.h>
#include <src/ScreenshotWidght/ScreenshotWidght.h>

#include <boost/signals2.hpp>
DOODLE_NAMESPACE_S
class AssScreenShotWidght : public ScreenshotWidght {
  Q_OBJECT
 public:
  AssScreenShotWidght(QWidget *parent = nullptr);

  void setAssInfo(const std::weak_ptr<assClass> &ass_class_ptr);

  void createScreenshot() override;
  void setimageInfo(const assInfoPtrList &list);

 private:
  std::weak_ptr<assClass> p_ass_class;
};

DOODLE_NAMESPACE_E