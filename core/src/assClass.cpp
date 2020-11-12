#include "assClass.h"
#include "coresql.h"

#include "assdepartment.h"

#include "znchName.h"

#include "Logger.h"

#include <memory>

#include <stdexcept>
#include "coreOrm/assclass_sqlOrm.h"
#include "coreOrm/znch_sqlOrm.h"
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>
#include <src/coreDataManager.h>


SQLPP_ALIAS_PROVIDER(znID)

CORE_NAMESPACE_S

assClass::assClass()
    : name(),
      p_assDep_id(-1),
      p_ass_dep_ptr_(),
      p_ptr_znch() {
}

void assClass::select(const qint64 &ID_) {
  doodle::Assclass table{};

  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row:db->run(
      sqlpp::select(sqlpp::all_of(table))
          .from(table)
          .where(table.id == ID_)
  )) {
    name = row.assName;
    idP = row.id;
    p_assDep_id = row.assdepId;
  }

}

void assClass::insert() {
  //id大于0就不逊要插入
  if (idP > 0) return;

  doodle::Assclass table{};

  auto db = coreSql::getCoreSql().getConnection();
  auto install = sqlpp::insert_into(table).columns(table.assName,
                                                   table.assdepId);
  install.values.add(table.assName = name,
                     table.assdepId = p_assDep_id);
  idP = db->insert(install);

  if (idP == 0) {
    throw std::runtime_error("not insert assclass");
  }
  if (p_ptr_znch)
    p_ptr_znch->insert();
  coreDataManager::get().setAssClassL(shared_from_this());
}

void assClass::updateSQL() {
  //转发只更新中文名称
  p_ptr_znch->updateSQL();
}

void assClass::deleteSQL() {
  if (p_ptr_znch)
    p_ptr_znch->deleteSQL();
  doodle::Assclass table{};

  auto db = coreSql::getCoreSql().getConnection();
  db->remove(sqlpp::remove_from(table)
                 .where(table.id == idP));
}

assClassPtrList assClass::getAll(const assDepPtr &ass_dep_ptr) {
  assClassPtrList list;

  doodle::Znch znNa{};
  doodle::Assclass table{};//.left_outer_join(znNa)
  auto db = coreSql::getCoreSql().getConnection();
  //sqlpp::all_of(table),sqlpp::all_of(znNa)


  for (auto &&row:db->run(
      sqlpp::select(table.id,table.assdepId,table.assName,znNa.localname,znNa.id.as(znID))
          .where(table.assdepId == ass_dep_ptr->getIdP())
          .from(table.left_outer_join(znNa).on(table.id == znNa.assClassId))
          .flags(sqlpp::distinct)
          .order_by(table.assName.asc())
  )) {
    auto assclass = std::make_shared<assClass>();

    assclass->name = row.assName;
    assclass->idP = row.id;
    assclass->setAssDep(ass_dep_ptr);
    if(row.localname._is_valid) {
//      std::cout << row.localname <<std::endl;
      assclass->p_ptr_znch = std::make_shared<znchName>(assclass.get());
      assclass->p_ptr_znch->nameZNCH = row.localname;
      assclass->p_ptr_znch->idP = row.znID;
      assclass->p_ptr_znch->nameEN = row.assName;
    }

    list.push_back(assclass);
  }
  coreDataManager::get().setAssClassL(list);
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
  }
  str = p_ptr_znch->getName();
  return str;
}

CORE_NAMESPACE_E
