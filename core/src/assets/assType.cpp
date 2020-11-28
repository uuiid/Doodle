/*
 * @Author: your name
 * @Date: 2020-11-06 13:15:08
 * @LastEditTime: 2020-11-28 14:54:55
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\assType.cpp
 */
//
// Created by teXiao on 2020/11/6.
//

#include "assType.h"
#include "src/core/coresql.h"
#include <src/core/coreset.h>
#include "assClass.h"

#include "Logger.h"

#include "src/coreOrm/asstype_sqlOrm.h"
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

#include <stdexcept>
#include <src/core/coreDataManager.h>
#include <magic_enum.hpp>
CORE_NAMESPACE_S
assType::assType()
    : coresqldata(),
      std::enable_shared_from_this<assType>(),
      s_type(),
      p_ass_class_id(-1) {}
void assType::insert() {
  if (idP > 0) return;
  // if (p_ass_class_id <= 0) return;

  doodle::Asstype table{};
  auto db = coreSql::getCoreSql().getConnection();
  auto insert =
      sqlpp::insert_into(table).columns(table.assType, table.projectId);
  insert.values.add(table.assType = s_type,
                    table.projectId = coreSet::getSet().projectName().first);

  idP = db->insert(insert);

  if (idP == 0) {
    DOODLE_LOG_WARN << "无法插入asstype " << s_type.c_str();
    throw std::runtime_error("asstype");
  }
  coreDataManager::get().setAssTypeL(shared_from_this());
}
void assType::updateSQL() {}
void assType::deleteSQL() {
  doodle::Asstype table{};
  auto db = coreSql::getCoreSql().getConnection();
  db->remove(sqlpp::remove_from(table).where(table.id == idP));
}
assTypePtrList assType::getAll() {
  assTypePtrList ptr_list;

  doodle::Asstype table{};
  auto db = coreSql::getCoreSql().getConnection();

  for (auto &&row : db->run(
           sqlpp::select(sqlpp::all_of(table))
               .from(table)
               .where(table.projectId == coreSet::getSet().projectName().first)
               .order_by(table.assType.desc()))) {
    auto at = std::make_shared<assType>();
    at->idP = row.id;
    at->s_type = row.assType;
    ptr_list.push_back(at);
  }
  coreDataManager::get().setAssTypeL(ptr_list);
  return ptr_list;
}
const std::string &assType::getType() const { return s_type; }
void assType::setType(const std::string &string) { assType::s_type = string; }
void assType::select(const int64_t &ID_) {}
bool assType::sortType(const assTypePtr &t1, const assTypePtr &t2) {
  return t1->s_type < t2->s_type;
}
assTypePtr assType::findType(const std::string &typeName) {
  for (const auto &item : coreDataManager::get().getAssTypeL()) {
    if (item->getType() == typeName) return item;
  }
  return nullptr;
}
assTypePtr assType::findType(const e_type &typeName, bool autoInstall) {
  std::string name{magic_enum::enum_name(typeName)};
  if (autoInstall) {
    auto asstype = findType(name);
    if (asstype)
      return asstype;
    else {
      auto type = std::make_shared<assType>();
      type->setType(name);
      type->insert();
      return type;
    }
  } else {
    return findType(name);
  }
}
CORE_NAMESPACE_E