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
#include <src/coreDataManager.h>
CORE_NAMESPACE_S

assdepartment::assdepartment()
    : i_prjID(-1),
      s_assDep("character") {
  i_prjID = coreSet::getSet().projectName().first;
}

void assdepartment::insert() {
  i_prjID = coreSet::getSet().projectName().first;

  auto db = coreSql::getCoreSql().getConnection();
  doodle::Assdepartment table{};

  auto insert = sqlpp::insert_into(table).columns(table.projectId,table.assDep);
  insert.values.add(table.projectId = i_prjID,
                    table.assDep = s_assDep);
  idP = db->insert(insert);
  if (idP == 0) {
    throw std::runtime_error("not install assDep");
  }
  coreDataManager::get().setAssDepL(shared_from_this());
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
  doodle::Assdepartment table{};
  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row:db->run(
      sqlpp::select(sqlpp::all_of(table))
          .from(table)
          .where(table.projectId == coreSet::getSet().projectName().first)
  ))
  {
    auto assdep = std::make_shared<assdepartment>();
    assdep->s_assDep = row.assDep.value();
    assdep->idP = row.id.value();
    assdep->i_prjID = row.projectId.value();
    list.push_back(assdep);
  }
  coreDataManager::get().setAssDepL(list);
  return list;

}
void assdepartment::select(const int64_t &ID_) {

}

CORE_NAMESPACE_E