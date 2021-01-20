#include "AssScreenShotWidght.h"

#include <core_Cpp.h>

DOODLE_NAMESPACE_S
AssScreenShotWidght::AssScreenShotWidght(QWidget *parent)
    : ScreenshotWidght(parent) {
}

void AssScreenShotWidght::setIndexInfo(const assInfoPtrList &list) {
  auto k_file =
      std::find_if(
          list.begin(), list.end(),
          [](assInfoPtr ass_) {
            return ass_->getAssType()->getType_enum() == assType::e_type::screenshot;
          });
  if (k_file != list.end()) {
    p_file_archive = (*k_file);
  }
}

DOODLE_NAMESPACE_E