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

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <corelib/coreOrm/pathStart_sqlOrm.h>

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
      p_assDep("character") {
  p_instance.insert(this);
}

assdepartment::~assdepartment() {
  p_instance.erase(this);
}

bool assdepartment::merge(const std::shared_ptr<assdepartment> &other) {
  auto it = std::find(p_alias.begin(), p_alias.end(), other->p_assDep);
  if (it == p_alias.end()) return false;

  for (auto &&item : other->p_roots)
    this->p_roots.push_back(item);
  return true;
}

bool assdepartment::setInfo(const std::string &value) {
  setAssDep(value);
  return true;
}

const std::string &assdepartment::getAssDep() const {
  return p_assDep;
}
void assdepartment::setAssDep(const std::string &s_ass_dep) {
  p_assDep = s_ass_dep;
}
assDepPtrList assdepartment::getAll() {
  auto &set = coreSet::getSet();

  auto db = coreSql::getCoreSql().getConnection();

  PathStart table{};
  auto sql = sqlpp::select(sqlpp::all_of(table))
                 .from(table)
                 .where(table.rootKey == "assRoot");
  //安装名称创建 dep
  assDepPtrList ass_list{};

  //这里获得dep的别名
  auto alias = coreSet::getSet().getProject()->getAlias(rttr::type::get<assdepartment>());
  for (auto &&item : set.getProject()->getClassNames(rttr::type::get<assdepartment>())) {
    ass_list.push_back(std::make_shared<assdepartment>());
    ass_list.back()->p_assDep = item.second;

    auto find_it = alias.equal_range(item.second);
    for (auto it = find_it.first; it != find_it.second; ++it) {
      ass_list.back()->p_alias.emplace_back(it->second);
    }
  }

  auto path_parser = set.getProject()->findParser(rttr::type::get<assdepartment>());

  for (auto &&row : (*db)(sql)) {
    int64_t k_id = row.id.value();
    auto it      = std::find_if(path_parser.begin(), path_parser.end(),
                           [=](std::shared_ptr<pathParser::PathParser> &parser) {
                             return parser->ID() == k_id;
                           });
    if (it == path_parser.end()) continue;

    //在这里解析一次路径  返回解析结果
    auto lists = (*it)->parser(fileSys::path{row.root.value()});
    for (auto &&it : lists) {
      for (auto &&k_ass : ass_list) {
        auto &k_it = it.get().get_value<assdepartment>();
        k_ass->merge(k_it.shared_from_this());
      }
    }
  }

  return ass_list;
}
std::unordered_set<assdepartment *> assdepartment::Instances() {
  return p_instance;
}

DOODLE_NAMESPACE_E
