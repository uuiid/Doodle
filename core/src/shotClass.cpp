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
    p_fileclass(e_fileclass::_),
    p_shot_id(-1),
    p_eps_id(-1),
    p_ptr_shot(),
    p_ptr_eps() {
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
    if (row.id._is_valid)
      p_shot_id = row.id;
    if (row.id._is_valid)
      p_eps_id = row.id;
  }

}

void shotClass::insert() {
  if (idP > 0) return;

  doodle::Shotclass table{};
  auto db = coreSql::getCoreSql().getConnection();
  auto install = sqlpp::dynamic_insert_into(*db, table)
      .dynamic_set(table.shotClass = sqlpp::value_or_null(getClass_str()));
//  auto test =   sqlpp::dynamic_insert_into(*db,table);
//  test.dynamic_set();

  if (p_ptr_shot)
    install.insert_list.add(table.shotsId = p_shot_id);
  if (p_ptr_eps)
    install.insert_list.add(table.episodesId = p_eps_id);

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
  if (row.id._is_valid)
    p_shot_id = row.id;
  if (row.id._is_valid)
    p_eps_id = row.id;
}

shotClassPtrList shotClass::getAll(const episodesPtr &episodes_ptr) {
  shotClassPtrList list{};

  doodle::Shotclass table{};
  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row:db->run(
      sqlpp::select(sqlpp::all_of(table))
          .from(table)
          .where(table.episodesId == episodes_ptr->getIdP())
  )) {
    auto item = std::make_shared<shotClass>();
    item->batchSetAttr(row);
    item->setEpisodes(episodes_ptr);
    list.push_back(item);
  }
  coreDataManager::get().setShotClassL(list);
  return list;
}

shotClassPtrList shotClass::getAll(const shotPtr &shot_ptr) {
  shotClassPtrList list{};

  doodle::Shotclass table{};
  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row:db->run(
      sqlpp::select(sqlpp::all_of(table))
          .from(table)
          .where(table.shotsId == shot_ptr->getIdP())
  )) {
    auto item = std::make_shared<shotClass>();
    item->batchSetAttr(row);
    item->setShot(shot_ptr);
    list.push_back(item);
  }
  coreDataManager::get().setShotClassL(list);
  return list;
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

episodesPtr shotClass::getEpisodes() {
  if (p_ptr_eps) {
    return p_ptr_eps;
  } else if (p_eps_id > 0) {
    episodesPtr p_ = std::make_shared<episodes>();
    p_->select(p_eps_id);
    p_ptr_eps = p_;
    return p_;
  } else {
    return nullptr;
  }
}

void shotClass::setEpisodes(const episodesPtr &value) {
  if (!value)
    return;
  p_ptr_eps = value;
  p_eps_id = value->getIdP();
}

shotPtr shotClass::getShot() {
  if (p_ptr_shot) {
    return p_ptr_shot;
  } else if (p_shot_id > 0) {
    shotPtr p_ = std::make_shared<shot>();
    p_->select(p_shot_id);
    p_ptr_shot = p_;
    return p_;
  } else {
    return nullptr;
  }
}

void shotClass::setShot(const shotPtr &value) {
  if (!value)
    return;
  p_ptr_shot = value;
  p_shot_id = value->getIdP();
  setEpisodes(value->getEpisodes());
}

CORE_NAMESPACE_E
