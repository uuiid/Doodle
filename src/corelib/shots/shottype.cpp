#include "shottype.h"

#include <loggerlib/Logger.h>

#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>

#include <corelib/core/coreset.h>
#include <corelib/core/coresql.h>
#include <corelib/shots/episodes.h>
#include <corelib/shots/shot.h>
#include <corelib/shots/shotClass.h>

#include <iostream>
#include <memory>

//反射使用
#include <rttr/registration>
DOODLE_NAMESPACE_S
RTTR_REGISTRATION {
  rttr::registration::class_<shotType>(DOCORE_RTTE_CLASS(shotType))
      .constructor<>()(rttr::policy::ctor::as_std_shared_ptr);
}
//初始化一些静态属性
DOODLE_INSRANCE_CPP(shotType);

boost::signals2::signal<void(const shotTypePtr &)> shotType::insertChanged{};
shotType::shotType()
    : CoreData(),
      std::enable_shared_from_this<shotType>(),
      p_shotClass_id(-1),
      p_Str_Type(),
      p_class_ptr_() {
  p_instance.insert(this);
}

shotType::~shotType() {
  p_instance.erase(this);
}
void shotType::select(const qint64 &ID_) {
}

void shotType::insert() {
}

void shotType::updateSQL() {
}

void shotType::deleteSQL() {
}

template <typename T>
void shotType::batchSetAttr(T &row) {
  idP            = row.id;
  p_Str_Type     = row.shotType;
  p_shotClass_id = row.shotClassId;
}

shotTypePtrList shotType::getAll() {
}

void shotType::setType(const dstring &value) { p_Str_Type = value; }

dstring shotType::getType() const { return p_Str_Type; }

void shotType::setShotClass(const shotClassPtr &fileclass_) {
  if (!fileclass_) return;
  p_shotClass_id = fileclass_->getIdP();
  p_class_ptr_   = fileclass_;
}

shotClassPtr shotType::getFileClass() {
  if (p_class_ptr_)
    return p_class_ptr_;
  else if (p_shotClass_id > 0) {
    for (auto &i : shotClass::Instances()) {
      if (i->getIdP() == p_shotClass_id) {
        p_class_ptr_ = i->shared_from_this();
        break;
      }
    }
    return p_class_ptr_;
  } else {
    return nullptr;
  }
}

shotTypePtr shotType::findShotType(const std::string &type_name) {
  shotTypePtr ptr = nullptr;
  // if (p_instance.empty()) shotType::getAll();
  //&&
  //        item->getFileClass() == shotClass::getCurrentClass()
  for (auto &item : p_instance) {
    if (item->getType() == type_name) {
      ptr = item->shared_from_this();
      break;
    }
  }
  return ptr;
}

shotTypePtr shotType::findShotType(const std::string &type_nmae,
                                   bool autoInstall) {
  shotTypePtr ptr = shotType::findShotType(type_nmae);
  if (!ptr) {
    ptr = std::make_shared<shotType>();
    ptr->setType(type_nmae);
    ptr->setShotClass(shotClass::getCurrentClass());
    ptr->insert();
  }
  return ptr;
}
const std::unordered_set<shotType *> shotType::Instances() {
  return p_instance;
}
DOODLE_NAMESPACE_E
