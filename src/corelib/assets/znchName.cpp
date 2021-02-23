/*
 * @Author: your name
 * @Date: 2020-10-19 13:26:31
 * @LastEditTime: 2020-12-14 13:30:35
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\znchName.cpp
 */
#include "znchName.h"

#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>

#include <stdexcept>

#include <loggerlib/Logger.h>
#include <corelib/assets/assClass.h>

#include <corelib/core/coreset.h>
#include <corelib/core/coresql.h>
#include <pinyinlib/convert.h>

//反射使用
#include <rttr/registration>

DOODLE_NAMESPACE_S

RTTR_REGISTRATION {
  rttr::registration::class_<znchName>(DOCORE_RTTE_CLASS(znchName))
      .constructor<assClass *>()(rttr::policy::ctor::as_std_shared_ptr);
}

znchName::znchName(assClass *at_)
    : CoreData(),
      nameEN(),
      nameZNCH(),
      p_ptr_assType(at_) {}

void znchName::setName(const std::string &name_) { nameEN = name_; }


void znchName::setName(const std::string &name_, const bool &isZNCH) {
  nameZNCH = name_;
  auto str = dopinyin::convert::Get().toEn(name_);

  if (str.size() > 18) {
    str.erase(str.begin() + 10, str.end() - 6);
  }

  nameEN = str;
}

std::string znchName::getName() const {
  if (!nameZNCH.empty())
    return nameZNCH;
  else if (!nameEN.empty())
    return nameEN;
  else
    return "";
}
std::string znchName::pinyin() const { return nameEN; }
DOODLE_NAMESPACE_E