#include "moveShotA.h"
#include "src/fileDBInfo/filesqlinfo.h"
#include "src/shots/shotfilesqlinfo.h"
#include <src/shots/shottype.h>
DOODLE_NAMESPACE_S
moveShotA::moveShotA(shotInfoPtr shot_info_ptr)
    : movieArchive(std::move(shot_info_ptr)) {
}

void moveShotA::insertDB() {
  p_info_ptr_ = std::get<shotInfoPtr>(p_info_ptr_->findSimilar());
  movieArchive::insertDB();
}
void moveShotA::setInfoAttr() {
  auto info = std::dynamic_pointer_cast<shotFileSqlInfo>(p_info_ptr_);
  if (p_info_ptr_->getInfoP().empty()) {
    p_info_ptr_->setInfoP("镜头拍屏");
  }
}
DOODLE_NAMESPACE_E