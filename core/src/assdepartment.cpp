//
// Created by teXiao on 2020/11/6.
//

#include "assdepartment.h"
#include "coreset.h"
#include "coresql.h"

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>
#include "coreOrm/assdepartment_sqlOrm.h"

#include <stdexcept>
#include <memory>
CORE_NAMESPACE_S

assdepartment::assdepartment()
    : i_prjID(-1),
      s_assDep("character") {
  i_prjID = coreSet::getCoreSet().projectName().first;
}

void assdepartment::insert() {
  i_prjID = coreSet::getCoreSet().projectName().first;

  auto db = coreSql::getCoreSql().getConnection();
  doodle::Assdepartment table;
  idP = db->insert(sqlpp::insert_into(table)
                       .set(table.projectId = i_prjID,
                            table.assDep = s_assDep));
  if (idP == 0) {
    throw std::runtime_error("not install assDep");
  }
}
void assdepartment::updateSQL() {

}
void assdepartment::deleteSQL() {

}
const std::string &assdepartment::getAssDep() const {
  return s_assDep;
}
void assdepartment::setAssDep(const std::string &s_ass_dep) {
  s_assDep = s_ass_dep;
}
assDepPtrList assdepartment::getAll() {
  assDepPtrList list;
  doodle::Assdepartment table;
  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row:db->run(
      sqlpp::select(sqlpp::all_of(table))
          .where(table.projectId == coreSet::getCoreSet().projectName().first)
  ))
  {
    auto assdep = std::make_shared<assdepartment>();
    assdep->s_assDep = row.assDep.value();
    assdep->idP = row.id.value();
    assdep->i_prjID = row.projectId.value();
    list.push_back(assdep);
  }
  return doCore::assDepPtrList();

}
void assdepartment::select(const int64_t &ID_) {

}

CORE_NAMESPACE_E