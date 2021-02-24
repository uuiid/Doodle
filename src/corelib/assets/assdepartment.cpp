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
#include <corelib/core/PathParser.h>

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
  auto &set = coreSet::getSet();

  auto roots       = set.getProject()->AssRoot();
  auto path_parser = set.getProject()->findParser(rttr::type::get<assdepartment>());

  assDepPtrList ass_list{};
  if (roots.size() == path_parser.size()) {
    for (size_t i = 0; i < roots.size(); ++i) {
      auto lists = path_parser[i]->parser(*roots[i]);
      for (auto &&item : lists) {
        if (item.get().can_convert(rttr::type::get<assdepartment>())) {
          auto &ass = item.get().get_value<assdepartment>();
          ass_list.push_back(ass.shared_from_this());
        }
      }
    }
  }
  return ass_list;
}
std::unordered_set<assdepartment *> assdepartment::Instances() {
  return p_instance;
}

DOODLE_NAMESPACE_E