/*
 * @Author: your name
 * @Date: 2020-11-10 10:28:32
 * @LastEditTime: 2020-12-01 13:59:48
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\fileArchive\moveShotA.cpp
 */
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
  if (p_info_ptr_->getInfoP().empty()) {
    p_info_ptr_->setInfoP("镜头拍屏");
  }
}
CORE_NAMESPACE_E