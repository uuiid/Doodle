//
// Created by teXiao on 2020/11/10.
//
#include "moveShotA.h"
#include "src/fileDBInfo/filesqlinfo.h"
#include "src/shots/shotfilesqlinfo.h"
#include <src/shots/shottype.h>
CORE_NAMESPACE_S
moveShotA::moveShotA(shotInfoPtr shot_info_ptr)
: movieArchive(std::move(shot_info_ptr)) {

}
void doCore::moveShotA::setInfoAttr() {
  auto info = std::dynamic_pointer_cast<shotFileSqlInfo>(p_info_ptr_);
  info->setInfoP("拍屏");

}
CORE_NAMESPACE_E