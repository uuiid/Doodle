/*
 * @Author: your name
 * @Date: 2020-11-06 09:22:09
 * @LastEditTime: 2020-12-14 16:41:51
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\assets\assdepartment.cpp
 */
//
// Created by teXiao on 2020/11/6.
//

#include "assdepartment.h"
#include <corelib/core/coreset.h>
#include <corelib/core/coresql.h>

//反射使用
#include <rttr/registration>
DOODLE_NAMESPACE_S

RTTR_REGISTRATION {
  rttr::registration::class_<assdepartment>(DOCORE_RTTE_CLASS(assdepartment))
      .constructor<>()(rttr::policy::ctor::as_std_shared_ptr);
}

DOODLE_INSRANCE_CPP(assdepartment);

assdepartment::assdepartment()
    : CoreData(),
      std::enable_shared_from_this<assdepartment>(),
      s_assDep("character") {
  p_instance.insert(this);
}

assdepartment::~assdepartment() {
  p_instance.erase(this);
}

bool assdepartment::setInfo(const std::string &value) {
  setAssDep(value);
  return true;
}

const std::string &assdepartment::getAssDep() const {
  return s_assDep;
}
void assdepartment::setAssDep(const std::string &s_ass_dep) {
  s_assDep = s_ass_dep;
}
assDepPtrList assdepartment::getAll() {
  return {};
}
std::unordered_set<assdepartment *> assdepartment::Instances() {
  return p_instance;
}

DOODLE_NAMESPACE_E