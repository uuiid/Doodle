#include "assClass.h"

#include <corelib/Exception/Exception.h>
#include <corelib/assets/assdepartment.h>
#include <corelib/assets/znchName.h>
#include <corelib/core/PathParser.h>
#include <corelib/core/coreset.h>
#include <corelib/core/Project.h>

#include <loggerlib/Logger.h>

//反射使用
#include <rttr/registration>

DOODLE_NAMESPACE_S

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
  setAssClass(value);
  return true;
}

assClassPtrList assClass::getAll(const assDepPtr &ass_dep_ptr) {
  // auto &set = coreSet::getSet();

  // auto roots       = ass_dep_ptr->Roots();
  // auto path_parser = set.getProject()->findParser(rttr::type::get<assClass>());

  // assClassPtrList ass_list{};

  // for (auto &parser : path_parser) {
  //   parser->getClassInstance.connect([]() { return std::make_shared<assClass>(); });
  //   for (auto &p_r : roots) {
  //     auto &ass_s = parser->parser(*p_r);
  //     for (auto &ass : ass_s) {
  //       ass_list.push_back(std::dynamic_pointer_cast<assClass>(ass));
  //       ass_list.back()->setAssDep(ass_dep_ptr);
  //     }
  //   }
  // }

  return {};
}

assDepPtr assClass::getAssDep() const {
  if (p_ass_dep_ptr_)
    return p_ass_dep_ptr_;
  else
    throw nullptr_error("assFileSqlInfo err");
}

void assClass::setAssDep(const assDepPtr &value) {
  p_ass_dep_ptr_ = value;
}

void assClass::setAssClass(const std::string &value) {
  name = value;
  // if (!p_ptr_znch) {
  //   p_ptr_znch = std::make_shared<znchName>(this);
  // }

  // p_ptr_znch->setName(value, true);
  // name = p_ptr_znch->pinyin();
}

std::string assClass::getAssClass() const {
  std::string str;
  // if (!p_ptr_znch)
  //   throw nullptr_error("");

  // str = p_ptr_znch->getName();
  return name;
}
const std::unordered_set<assClass *> assClass::Instances() {
  return p_instance;
}
DOODLE_NAMESPACE_E
