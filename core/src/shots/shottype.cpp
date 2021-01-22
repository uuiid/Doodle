#include "shottype.h"

#include <Logger.h>
#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>
#include <src/coreOrm/shottype_sqlOrm.h>
#include <src/core/coreset.h>
#include <src/core/coresql.h>
#include <src/shots/episodes.h>
#include <src/shots/shot.h>
#include <src/shots/shotClass.h>

#include <iostream>
#include <memory>

//反射使用
#include <rttr/registration>
DOODLE_NAMESPACE_S
RTTR_REGISTRATION {
  rttr::registration::class_<shotType>(DOCORE_RTTE_CLASS(shotType))
      .constructor<>()(rttr::policy::ctor::as_std_shared_ptr);
}
//初始化一些静态属性
DOODLE_INSRANCE_CPP(shotType);

boost::signals2::signal<void(const shotTypePtr &)> shotType::insertChanged{};
shotType::shotType()
    : coresqldata(),
      std::enable_shared_from_this<shotType>(),
      p_shotClass_id(-1),
      p_Str_Type(),
      p_class_ptr_() {
  p_instance.insert(this);
}

shotType::~shotType() {
  p_instance.erase(this);
}
void shotType::select(const qint64 &ID_) {
  doodle::Shottype table{};

  auto db = coreSql::getCoreSql().getConnection();

  for (auto &&row : db->run(sqlpp::select(sqlpp::all_of(table))
                                .from(table)
                                .where(table.id == ID_))) {
    batchSetAttr(row);
  }
}

void shotType::insert() {
  if (isInsert()) return;
  doodle::Shottype table{};

  auto db     = coreSql::getCoreSql().getConnection();
  auto insert = sqlpp::insert_into(table).columns(
      table.shotType, table.shotClassId, table.projectId);
  insert.values.add(table.shotType    = p_Str_Type,
                    table.shotClassId = p_shotClass_id,
                    table.projectId   = coreSet::getSet().projectName().first);

  idP = db->insert(insert);
  if (idP == 0) {
    DOODLE_LOG_WARN("无法插入shot type " << p_Str_Type.c_str());
    throw std::runtime_error("not install shot type");
  }
  insertChanged(shared_from_this());
}

void shotType::updateSQL() {
  if (isNULL()) return;
  doodle::Shottype table{};

  auto db = coreSql::getCoreSql().getConnection();

  db->update(sqlpp::update(table)
                 .set(table.shotType = p_Str_Type)
                 .where(table.id == idP));
}

void shotType::deleteSQL() {
  doodle::Shottype table{};

  auto db = coreSql::getCoreSql().getConnection();
  db->remove(sqlpp::remove_from(table).where(table.id == idP));
}

template <typename T>
void shotType::batchSetAttr(T &row) {
  idP            = row.id;
  p_Str_Type     = row.shotType;
  p_shotClass_id = row.shotClassId;
}

shotTypePtrList shotType::getAll() {
  doodle::Shottype table{};

  auto db = coreSql::getCoreSql().getConnection();
  shotTypePtrList list;
  for (auto &&row : db->run(
           sqlpp::select(sqlpp::all_of(table))
               .from(table)
               .where(table.projectId == coreSet::getSet().projectName().first)
               .order_by(table.shotType.desc()))) {
    auto item = std::make_shared<shotType>();
    item->batchSetAttr(row);
    auto k_shot_instance = shotClass::Instances();
    auto find_item =
        std::find_if(
            k_shot_instance.begin(), k_shot_instance.end(),
            [=](shotClass *ptr)
                -> bool { return ptr->getIdP() == item->p_shotClass_id; });

    if (find_item != k_shot_instance.end())
      item->setShotClass((*find_item)->shared_from_this());
    list.push_back(item);
  }

  return list;
}

void shotType::setType(const dstring &value) { p_Str_Type = value; }

dstring shotType::getType() const { return p_Str_Type; }

void shotType::setShotClass(const shotClassPtr &fileclass_) {
  if (!fileclass_) return;
  p_shotClass_id = fileclass_->getIdP();
  p_class_ptr_   = fileclass_;
}

shotClassPtr shotType::getFileClass() {
  if (p_class_ptr_)
    return p_class_ptr_;
  else if (p_shotClass_id > 0) {
    for (auto &i : shotClass::Instances()) {
      if (i->getIdP() == p_shotClass_id) {
        p_class_ptr_ = i->shared_from_this();
        break;
      }
    }
    return p_class_ptr_;
  } else {
    return nullptr;
  }
}

shotTypePtr shotType::findShotType(const std::string &type_name) {
  shotTypePtr ptr = nullptr;
  // if (p_instance.empty()) shotType::getAll();
  //&&
  //        item->getFileClass() == shotClass::getCurrentClass()
  for (auto &item : p_instance) {
    if (item->getType() == type_name) {
      ptr = item->shared_from_this();
      break;
    }
  }
  return ptr;
}

shotTypePtr shotType::findShotType(const std::string &type_nmae,
                                   bool autoInstall) {
  shotTypePtr ptr = shotType::findShotType(type_nmae);
  if (!ptr) {
    ptr = std::make_shared<shotType>();
    ptr->setType(type_nmae);
    ptr->setShotClass(shotClass::getCurrentClass());
    ptr->insert();
  }
  return ptr;
}
const std::unordered_set<shotType *> shotType::Instances() {
  return p_instance;
}
DOODLE_NAMESPACE_E
