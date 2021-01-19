#include "AssScreenShotWidght.h"

#include <core_Cpp.h>

DOODLE_NAMESPACE_S
AssScreenShotWidght::AssScreenShotWidght(QWidget *parent)
    : ScreenshotWidght(parent) {
}

void AssScreenShotWidght::setIndexInfo(const assClassPtr &ass_class_ptr) {
  auto list = assFileSqlInfo::Instances();
  auto k_file =
      std::find_if(list.begin(), list.end(),
                   [ass_class_ptr](assFileSqlInfo *ass_) {
                     return ass_->getAssClass() == ass_class_ptr &&
                            ass_->getAssType()->getTypeS() == "";
                   });
}

DOODLE_NAMESPACE_E