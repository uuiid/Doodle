#include "AssScreenShotWidght.h"
#include <src/Exception/Exception.h>
#include <core_Cpp.h>

DOODLE_NAMESPACE_S
AssScreenShotWidght::AssScreenShotWidght(QWidget *parent)
    : ScreenshotWidght(parent),
      p_ass_class() {
}

void AssScreenShotWidght::setAssInfo(const std::weak_ptr<assClass> &ass_class_ptr) {
  p_ass_class = ass_class_ptr;
}

void AssScreenShotWidght::createScreenshot() {
  if (p_ass_class.expired()) throw nullptr_error("没有 ass指针");

  auto k_fileArch = std::make_shared<assFileSqlInfo>();
  k_fileArch->setAssClass(p_ass_class.lock());
  k_fileArch->setAssType(assType::findType(assType::e_type::screenshot, true));

  k_fileArch     = std::get<assInfoPtr>(k_fileArch->findSimilar());
  p_file_archive = k_fileArch;
  ScreenshotWidght::createScreenshot();
}

void AssScreenShotWidght::setimageInfo(const assInfoPtrList &list) {
  auto k_file =
      std::find_if(
          list.begin(), list.end(),
          [](assInfoPtr ass_) {
            return ass_->getAssType()->getType_enum() == assType::e_type::screenshot;
          });
  if (k_file != list.end()) {
    p_file_archive = (*k_file);
    showImage();
  } else {
    clearImage();
  }
}

DOODLE_NAMESPACE_E