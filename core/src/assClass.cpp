
#include "coreset.h"
#include "coresql.h"

#include "assClass.h"
#include "assdepartment.h"
#include "znchName.h"

#include "Logger.h"

#include <memory>
#include <stdexcept>

#include "coreOrm/assclass_sqlOrm.h"
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

CORE_NAMESPACE_S

assClass::assClass()
    : name(),
      p_assDep_id(-1),
      p_ass_dep_ptr_(),
      p_ptr_znch() {
}

void assClass::select(const qint64 &ID_) {
  doodle::Assclass table;
  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row:db->run(
      sqlpp::select(sqlpp::all_of(table))
          .where(table.assdepId == ID_)
  )) {
    name = row.assName;
    idP = row.id;
    p_assDep_id = row.assdepId;
  }
}

void assClass::insert() {
  //id大于0就不逊要插入
  if (idP > 0) return;

  doodle::Assclass table;
  auto db = coreSql::getCoreSql().getConnection();
  idP = db->insert(sqlpp::insert_into(table).set(table.assName = name));
  if (idP == 0) {
    throw std::runtime_error("not insert assclass");
  }
  if (p_ptr_znch)
    p_ptr_znch->insert();
}

void assClass::updateSQL() {
  //转发只更新中文名称
  p_ptr_znch->updateSQL();
}

void assClass::deleteSQL() {
}

assClassPtrList assClass::getAll(const assDepPtr &ass_dep_ptr) {
  assClassPtrList list;

  doodle::Assclass table;
  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row:db->run(
      sqlpp::select(sqlpp::all_of(table))
          .where(table.assdepId == ass_dep_ptr->getIdP())
  )) {
    auto assclass = std::make_shared<assClass>();

    assclass->name = row.assName;
    assclass->idP = row.id;
    assclass->setAssDep(ass_dep_ptr);

    list.push_back(assclass);
  }
  return list;
}

assDepPtr assClass::getAssDep() const {
  if (p_ass_dep_ptr_)
    return p_ass_dep_ptr_;
  else
    return nullptr;

}

void assClass::setAssDep(const assDepPtr &value) {
  p_ass_dep_ptr_ = value;
  p_assDep_id = value->getIdP();
}

void assClass::setAssClass(const std::string &value) {
  if (!p_ptr_znch) {
    p_ptr_znch = std::make_shared<znchName>(this);
  }

  p_ptr_znch->setName(value, true);
  name = p_ptr_znch->pinyin();
}

void assClass::setAssClass(const std::string &value, const bool &isZNCH) {
  setAssClass(value);
}

std::string assClass::getAssClass() const {
  return name;
}
std::string assClass::getAssClass(const bool &isZNCH) {
  std::string str;
  if (!p_ptr_znch) {
    p_ptr_znch = std::make_shared<znchName>(this);
    p_ptr_znch->select();
  }
  str = p_ptr_znch->getName();
  return str;
}

CORE_NAMESPACE_E
