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

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

#include <stdexcept>
#include <memory>
#include <QString>

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
      i_prjID(-1),
      s_assDep("character") {
  i_prjID = coreSet::getSet().projectName().first;
  p_instance.insert(this);
}

assdepartment::~assdepartment() {
  p_instance.erase(this);
}

void assdepartment::insert() {
}
void assdepartment::updateSQL() {
}
void assdepartment::deleteSQL() {
}
const std::string &assdepartment::getAssDep() const {
  return s_assDep;
}
void assdepartment::setAssDep(const std::string &s_ass_dep) {
  s_assDep = s_ass_dep;
}
assDepPtrList assdepartment::getAll() {
}
std::unordered_set<assdepartment *> assdepartment::Instances() {
  return p_instance;
}
const QString assdepartment::getAssDepQ() const {
  return QString::fromStdString(getAssDep());
}

DOODLE_NAMESPACE_E