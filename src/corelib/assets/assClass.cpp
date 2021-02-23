#include "assClass.h"
#include <corelib/core/coresql.h>

#include <corelib/assets/assdepartment.h>

#include <corelib/assets/znchName.h>

#include <loggerlib/Logger.h>

#include <stdexcept>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

//反射使用
#include <rttr/registration>
#include <memory>

//注册sql库使用的外键

DOODLE_NAMESPACE_S
SQLPP_ALIAS_PROVIDER(znID)

RTTR_REGISTRATION {
  rttr::registration::class_<assClass>(DOCORE_RTTE_CLASS(assClass))
      .constructor<>()(rttr::policy::ctor::as_std_shared_ptr);
}

DOODLE_INSRANCE_CPP(assClass);
assClass::assClass()
    : CoreData(),
      std::enable_shared_from_this<assClass>(),
      name(),
      p_ass_dep_ptr_(),
      p_ptr_znch() {
  p_instance.insert(this);
}

assClass::~assClass() {
  p_instance.erase(this);
}

bool assClass::setInfo(const std::string &value) {
}

assClassPtrList assClass::getAll(const assDepPtr &ass_dep_ptr) {
}

assDepPtr assClass::getAssDep() const {
  if (p_ass_dep_ptr_)
    return p_ass_dep_ptr_;
  else
    return nullptr;
}

void assClass::setAssDep(const assDepPtr &value) {
  p_ass_dep_ptr_ = value;
}

void assClass::setAssClass(const std::string &value) {
  if (!p_ptr_znch) {
    p_ptr_znch = std::make_shared<znchName>(this);
  }

  p_ptr_znch->setName(value, true);
  name = p_ptr_znch->pinyin();
}

void assClass::setAssClass(const std::string &value, const bool &isZNCH) {
  setAssClass(value);
}

std::string assClass::getAssClass() const { return name; }
std::string assClass::getAssClass(const bool &isZNCH) {
  std::string str;
  if (isZNCH) {
    if (!p_ptr_znch) {
      p_ptr_znch = std::make_shared<znchName>(this);
    }
    str = p_ptr_znch->getName();
  } else {
    str = name;
  }

  return str;
}
const std::unordered_set<assClass *> assClass::Instances() {
  return p_instance;
}
DOODLE_NAMESPACE_E
