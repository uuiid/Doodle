/*
 * @Author: your name
 * @Date: 2020-11-06 13:15:08
 * @LastEditTime: 2020-12-14 16:44:22
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\assType.cpp
 */
//
// Created by teXiao on 2020/11/6.
//

#include "assType.h"
#include <corelib/core/coresql.h>
#include <corelib/core/coreset.h>
#include <corelib/assets/assClass.h>

#include <loggerlib/Logger.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

#include <stdexcept>
#include <magic_enum.hpp>

//反射使用
#include <rttr/registration>

DOODLE_NAMESPACE_S
RTTR_REGISTRATION {
  rttr::registration::class_<assType>(DOCORE_RTTE_CLASS(assType))
      .constructor<>()(rttr::policy::ctor::as_std_shared_ptr);
}

DOODLE_INSRANCE_CPP(assType);
boost::signals2::signal<void()> assType::insertChanged{};

assType::assType()
    : CoreData(),
      std::enable_shared_from_this<assType>(),
      s_type(),
      p_ass_class_id(-1) {
  p_instance.insert(this);
}

assType::~assType() {
  p_instance.erase(this);
}
void assType::insert() {
}
void assType::updateSQL() {}
void assType::deleteSQL() {
}
assTypePtrList assType::getAll() {
}

const std::string assType::getTypeS() const {
  return std::string{magic_enum::enum_name(s_type)};
}

const assType::e_type &assType::getType_enum() const {
  return s_type;
}

void assType::setType(const e_type &type_enum) {
  s_type = type_enum;
}

void assType::setType(const std::string &string) {
  s_type = magic_enum::enum_cast<e_type>(string).value_or(e_type::None);
}

bool assType::sortType(const assTypePtr &t1, const assTypePtr &t2) {
  return t1->s_type < t2->s_type;
}
assTypePtr assType::findType(const std::string &typeName) {
  for (const auto &item : p_instance) {
    if (item->getTypeS() == typeName) return item->shared_from_this();
  }
  return nullptr;
}
assTypePtr assType::findType(const e_type &typeName, bool autoInstall) {
  std::string name{magic_enum::enum_name(typeName)};
  auto k_asstype =
      std::find_if(
          p_instance.begin(),
          p_instance.end(),
          [=](assType *as_tmp)
              -> bool {
            return as_tmp->s_type == typeName;
          });

  if (k_asstype != p_instance.end())
    return (*k_asstype)->shared_from_this();
  else {
    if (autoInstall) {
      auto type = std::make_shared<assType>();
      type->setType(typeName);
      type->insert();
      return type;
    }
  }
  return nullptr;
}

const std::unordered_set<assType *> assType::Instances() {
  return p_instance;
}
DOODLE_NAMESPACE_E