#include "shottype.h"

#include <loggerlib/Logger.h>

#include <corelib/core/coreset.h>
#include <corelib/core/coresql.h>
#include <corelib/shots/episodes.h>
#include <corelib/shots/shot.h>
#include <corelib/shots/shotClass.h>
#include <corelib/Exception/Exception.h>

DOODLE_NAMESPACE_S

//初始化一些静态属性
DOODLE_INSRANCE_CPP(shotType);

boost::signals2::signal<void(const shotTypePtr &)> shotType::insertChanged{};
shotType::shotType()
    : CoreData(),
      std::enable_shared_from_this<shotType>(),
      p_Str_Type(),
      p_class_ptr_() {
  p_instance.insert(this);
}

shotType::~shotType() {
  p_instance.erase(this);
}

shotTypePtrList shotType::getAll() {
  return {};
}

void shotType::setType(const dstring &value) { p_Str_Type = value; }

dstring shotType::getType() const { return p_Str_Type; }

void shotType::setShotClass(const shotClassPtr &fileclass_) {
  if (!fileclass_) return;
  p_class_ptr_ = fileclass_;
}

shotClassPtr shotType::getFileClass() {
  if (p_class_ptr_)
    return p_class_ptr_;
  else
    throw nullptr_error("shotClassPtr err");
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
bool shotType::setInfo(const std::string &value) {
  return true;
}

shotTypePtr shotType::findShotType(const std::string &type_nmae,
                                   bool autoInstall) {
  shotTypePtr ptr = shotType::findShotType(type_nmae);
  if (!ptr) {
    ptr = std::make_shared<shotType>();
    ptr->setType(type_nmae);
    ptr->setShotClass(shotClass::getCurrentClass());
  }
  return ptr;
}
const std::unordered_set<shotType *> shotType::Instances() {
  return p_instance;
}
DOODLE_NAMESPACE_E
