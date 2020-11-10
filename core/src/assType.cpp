//
// Created by teXiao on 2020/11/6.
//

#include "assType.h"
#include "coresql.h"
#include "assClass.h"

#include "Logger.h"

#include "coreOrm/asstype_sqlOrm.h"
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

#include <stdexcept>

CORE_NAMESPACE_S
assType::assType()
    : s_type(),
      p_ass_class_id(-1),
      p_class_ptr_() {
}
void assType::insert() {
  if (idP > 0) return;
  if (p_ass_class_id <= 0) return;
  if (!p_class_ptr_) return;

  doodle::Asstype table{};
  auto db = coreSql::getCoreSql().getConnection();
  auto insert = sqlpp::insert_into(table).columns(table.assType,
                                                  table.assClassId);
  insert.values.add(table.assType = s_type,
                  table.assClassId = p_ass_class_id);

  idP = db->insert(insert);

  if (idP == 0) {
    DOODLE_LOG_WARN << "无法插入asstype " << s_type.c_str();
    throw std::runtime_error("asstype");
  }
}
void assType::updateSQL() {

}
void assType::deleteSQL() {
  doodle::Asstype table{};
  auto db = coreSql::getCoreSql().getConnection();
  db->remove(sqlpp::remove_from(table)
                 .where(table.id == idP));
}
assTypePtrList assType::getAll(const assClassPtr &ass_class_ptr) {
  assTypePtrList ptr_list;

  doodle::Asstype table{};
  auto db = coreSql::getCoreSql().getConnection();

  for (auto &&row:db->run(
      sqlpp::select(sqlpp::all_of(table))
          .from(table)
          .where(table.assClassId == ass_class_ptr->getIdP())
  )) {
    auto at = std::make_shared<assType>();
    at->idP = row.id;
    at->s_type = row.assType;
    at->setAssClassPtr(ass_class_ptr);
    ptr_list.push_back(at);
  }
  return ptr_list;
}
const std::string &assType::getType() const {
  return s_type;
}
void assType::setType(const std::string &string) {
  assType::s_type = string;
}
const assClassPtr &assType::getAssClassPtr() const {
  return p_class_ptr_;
}
void assType::setAssClassPtr(const assClassPtr &class_ptr) {
  p_ass_class_id = class_ptr->getIdP();
  p_class_ptr_ = class_ptr;
}
void assType::select(const int64_t &ID_) {

}
CORE_NAMESPACE_E