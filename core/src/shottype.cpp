#include "shottype.h"

#include "coresql.h"

#include "shot.h"
#include "episodes.h"
#include "shotClass.h"
#include <src/coreset.h>
#include "coreOrm/shottype_sqlOrm.h"
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

#include "Logger.h"

#include <iostream>
#include <memory>
#include <src/coreDataManager.h>

CORE_NAMESPACE_S

shotType::shotType()
    : coresqldata(),
      std::enable_shared_from_this<shotType>(),
      p_shotClass_id(-1),
      p_Str_Type(),
      p_class_ptr_() {
}

void shotType::select(const qint64 &ID_) {
  doodle::Shottype table{};

  auto db = coreSql::getCoreSql().getConnection();

  for (auto &&row:db->run(
      sqlpp::select(sqlpp::all_of(table))
          .from(table)
          .where(table.id == ID_)
  )) {
    batchSetAttr(row);
  }
}

void shotType::insert() {
  if (isInsert()) return;
  doodle::Shottype table{};

  auto db = coreSql::getCoreSql().getConnection();
  auto insert = sqlpp::insert_into(table)
      .columns(table.shotType,
               table.shotClassId,
               table.projectId);
  insert.values.add(table.shotType = p_Str_Type,
                    table.shotClassId = p_shotClass_id,
                    table.projectId = coreSet::getSet().projectName().first);

  idP = db->insert(insert);
  if (idP == 0) {
    DOODLE_LOG_WARN << "无法插入shot type " << p_Str_Type.c_str();
    throw std::runtime_error("not install shot type");
  }
  coreDataManager::get().setShotTypeL(shared_from_this());
}

void shotType::updateSQL() {
  if (isNULL()) return;
  doodle::Shottype table{};

  auto db = coreSql::getCoreSql().getConnection();

  db->update(
      sqlpp::update(table)
          .set(table.shotType = p_Str_Type)
          .where(table.id == idP)
  );
}

void shotType::deleteSQL() {
  doodle::Shottype table{};

  auto db = coreSql::getCoreSql().getConnection();
  db->remove(sqlpp::remove_from(table)
                 .where(table.id == idP));
}

template<typename T>
void shotType::batchSetAttr(T &row) {
  idP = row.id;
  p_Str_Type = row.shotType;
  p_shotClass_id = row.shotClassId;
}

shotTypePtrList shotType::getAll() {
  doodle::Shottype table{};
  const auto shotclasList = coreDataManager::get().getShotClassL();

  auto db = coreSql::getCoreSql().getConnection();
  shotTypePtrList list;
  for (auto &&row:db->run(
      sqlpp::select(sqlpp::all_of(table))
          .from(table)
          .where(table.projectId == coreSet::getSet().projectName().first)
  )) {
    auto item = std::make_shared<shotType>();
    item->batchSetAttr(row);
    for (const auto &shCl : shotclasList) {
      if(item->p_shotClass_id == shCl->getIdP())
        item->setShotClass(shCl);
    }
    list.push_back(item);
  }
  coreDataManager::get().setShotTypeL(list);
  return list;
}

void shotType::setType(const dstring &value) {
  p_Str_Type = value;
}

dstring shotType::getType() const {
  return p_Str_Type;
}

void shotType::setShotClass(const shotClassPtr &fileclass_) {
  if (!fileclass_)
    return;
  p_shotClass_id = fileclass_->getIdP();
  p_class_ptr_ = fileclass_;
}

shotClassPtr shotType::getFileClass() {
  if (p_class_ptr_)
    return p_class_ptr_;
  else if (p_shotClass_id > 0) {
    shotClassPtr p_ = std::make_shared<shotClass>();
    p_->select(p_shotClass_id);
    p_class_ptr_ = p_;
    return p_;
  } else {
    return nullptr;
  }
}


CORE_NAMESPACE_E
