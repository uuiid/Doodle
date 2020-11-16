//
// Created by teXiao on 2020/11/6.
//

#include "assType.h"
#include "coresql.h"
#include <src/coreset.h>
#include "assClass.h"

#include "Logger.h"

#include "coreOrm/asstype_sqlOrm.h"
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

#include <stdexcept>
#include <src/coreDataManager.h>
CORE_NAMESPACE_S
assType::assType()
    : coresqldata(),
      std::enable_shared_from_this<assType>(),
      s_type(),
      p_ass_class_id(-1) {
}
void assType::insert() {
  if (idP > 0) return;
  if (p_ass_class_id <= 0) return;

  doodle::Asstype table{};
  auto db = coreSql::getCoreSql().getConnection();
  auto insert = sqlpp::insert_into(table).columns(table.assType,
                                                  table.projectId);
  insert.values.add(table.assType = s_type,
                    table.projectId = coreSet::getSet().projectName().first);

  idP = db->insert(insert);

  if (idP == 0) {
    DOODLE_LOG_WARN << "无法插入asstype " << s_type.c_str();
    throw std::runtime_error("asstype");
  }
  coreDataManager::get().setAssTypeL(shared_from_this());
}
void assType::updateSQL() {

}
void assType::deleteSQL() {
  doodle::Asstype table{};
  auto db = coreSql::getCoreSql().getConnection();
  db->remove(sqlpp::remove_from(table)
                 .where(table.id == idP));
}
assTypePtrList assType::getAll() {
  assTypePtrList ptr_list;

  doodle::Asstype table{};
  auto db = coreSql::getCoreSql().getConnection();

  for (auto &&row:db->run(
      sqlpp::select(sqlpp::all_of(table))
          .from(table)
          .where(table.projectId == coreSet::getSet().projectName().first)
  )) {
    auto at = std::make_shared<assType>();
    at->idP = row.id;
    at->s_type = row.assType;
    ptr_list.push_back(at);
  }
  coreDataManager::get().setAssTypeL(ptr_list);
  return ptr_list;
}
const std::string &assType::getType() const {
  return s_type;
}
void assType::setType(const std::string &string) {
  assType::s_type = string;
}
void assType::select(const int64_t &ID_) {

}
CORE_NAMESPACE_E