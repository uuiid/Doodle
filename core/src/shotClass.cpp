#include "shotClass.h"

#include "coreset.h"
#include "episodes.h"
#include "shot.h"
#include "coresql.h"

#include "coreOrm/shotclass_sqlOrm.h"
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

#include "magic_enum.hpp"

#include <iostream>
#include <memory>
#include <Logger.h>

#include <src/coreDataManager.h>

CORE_NAMESPACE_S

shotClass::shotClass() :
    coresqldata(),
    std::enable_shared_from_this<shotClass>(),
    p_fileclass(e_fileclass::_) {
}

void shotClass::select(const qint64 &ID_) {
  doodle::Shotclass table{};
  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row: db->run(
      sqlpp::select(sqlpp::all_of(table))
          .from(table)
          .where(table.id == ID_)
  )) {
    idP = row.id;
    setclass(row.shotClass);
  }

}

void shotClass::insert() {
  if (idP > 0) return;

  doodle::Shotclass table{};
  auto db = coreSql::getCoreSql().getConnection();
  auto install = sqlpp::insert_into(table)
      .set(
          table.shotClass = getClass_str(),
          table.projectId = coreSet::getSet().projectName().first
          );
  idP = db->insert(install);
  if (idP == 0) {
    DOODLE_LOG_WARN << "无法插入shot type" << getClass_str().c_str();
    throw std::runtime_error("not install shot");
  }
  coreDataManager::get().setShotClassL(shared_from_this());
}

void shotClass::updateSQL() {
  if (idP < 0) return;
  doodle::Shotclass table{};
  auto db = coreSql::getCoreSql().getConnection();
  db->update(sqlpp::update(table)
                 .set(table.shotClass = getClass_str())
                 .where(table.id == idP));
}

void shotClass::deleteSQL() {
  doodle::Shotclass table{};
  auto db = coreSql::getCoreSql().getConnection();
  db->remove(sqlpp::remove_from(table)
                 .where(table.id == idP));
}
template<typename T>
void shotClass::batchSetAttr(T &row) {
  idP = row.id;
  setclass(row.shotClass);
}

shotClassPtrList shotClass::getAll() {
  shotClassPtrList list{};

  doodle::Shotclass table{};
  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row:db->run(
      sqlpp::select(sqlpp::all_of(table))
          .from(table)
          .where(table.projectId == coreSet::getSet().projectName().first)
          .order_by(table.shotClass.desc())
  )) {
    auto item = std::make_shared<shotClass>();
    item->batchSetAttr(row);
    list.push_back(item);
  }
  coreDataManager::get().setShotClassL(list);
  return list;
}

shotClassPtr shotClass::getCurrentClass() { 
  shotClassPtr ptr = nullptr;
  for (auto &item : coreDataManager::get().getShotClassL()) {
    if (item->getClass_str() == coreSet::getSet().getDepartment()) {
      ptr = item;
      break;
    }
  }
  if (!ptr) {
    ptr = std::make_shared<shotClass>();
    ptr->setclass(coreSet::getSet().getDepartment());
    ptr->insert();
  }

  return ptr; 

}

dstring shotClass::getClass_str() const {
  std::string str(magic_enum::enum_name(p_fileclass));
  return str;
}

shotClass::e_fileclass shotClass::getClass() const {
  return p_fileclass;
}

void shotClass::setclass(const e_fileclass &value) {
  p_fileclass = value;
}

void shotClass::setclass(const dstring &value) {
  auto tmp_fc = magic_enum::enum_cast<e_fileclass>(value);
  if (tmp_fc.has_value()) {
    p_fileclass = tmp_fc.value();
  } else {
    throw std::runtime_error("not file class in enum");
  }
}

CORE_NAMESPACE_E
