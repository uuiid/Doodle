#include "shotClass.h"

#include <corelib/Exception/Exception.h>

#include <loggerlib/Logger.h>
#include <corelib/core/coreset.h>
#include <corelib/core/coresql.h>
#include <corelib/shots/episodes.h>
#include <corelib/shots/shot.h>

#include <magic_enum.hpp>

DOODLE_NAMESPACE_S

DOODLE_INSRANCE_CPP(shotClass);
boost::signals2::signal<void(const shotClassPtr &)> shotClass::insertChanged{};

shotClass::shotClass()
    : CoreData(),
      std::enable_shared_from_this<shotClass>(),
      p_fileclass(e_fileclass::_) {
  p_instance.insert(this);
}

shotClass::~shotClass() {
  p_instance.erase(this);
}

bool shotClass::setInfo(const std::string &value) {
  return true;
}

shotClassPtrList shotClass::getAll() {
  return {};
}

shotClassPtr shotClass::getCurrentClass() {
  shotClassPtr ptr = nullptr;
  for (auto &item : p_instance) {
    DOODLE_LOG_DEBUG("current class " << item->getClass_str() << " current dep " << coreSet::getSet().getDepartment());
    if (item->getClass_str() == coreSet::getSet().getDepartment()) {
      ptr = item->shared_from_this();
      break;
    }
  }
  if (!ptr) {
    DOODLE_LOG_ERROR("find not shot class " << coreSet::getSet().getDepartment())
    throw DoodleError("");
  }

  return ptr;
}

dstring shotClass::getClass_str() const {
  std::string str(magic_enum::enum_name(p_fileclass));
  return str;
}

shotClass::e_fileclass shotClass::getClass() const { return p_fileclass; }

void shotClass::setclass(const e_fileclass &value) { p_fileclass = value; }

void shotClass::setclass(const dstring &value) {
  auto tmp_fc = magic_enum::enum_cast<e_fileclass>(value);
  if (tmp_fc.has_value()) {
    p_fileclass = tmp_fc.value();
  } else {
    throw DoodleError("not file class in enum");
  }
}
const std::unordered_set<shotClass *> shotClass::Instances() {
  return p_instance;
}
DOODLE_NAMESPACE_E
