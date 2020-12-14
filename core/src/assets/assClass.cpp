#include "assClass.h"
#include <src/core/coresql.h>

#include <src/assets/assdepartment.h>

#include <src/assets/znchName.h>

#include <Logger.h>

#include <stdexcept>
#include <src/coreOrm/assclass_sqlOrm.h>
#include <src/coreOrm/znch_sqlOrm.h>
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>
#include <src/core/coreDataManager.h>

//反射使用
#include <rttr/registration>
#include <memory>

//注册sql库使用的外键
SQLPP_ALIAS_PROVIDER(znID)

CORE_NAMESPACE_S

RTTR_REGISTRATION {
  rttr::registration::class_<assClass>(DOCORE_RTTE_CLASS(assClass))
      .constructor<>()(rttr::policy::ctor::as_std_shared_ptr);
}

assClass::assClass()
    : coresqldata(),
      std::enable_shared_from_this<assClass>(),
      name(),
      p_assDep_id(-1),
      p_ass_dep_ptr_(),
      p_ptr_znch() {}

void assClass::insert() {
  // id大于0就不逊要插入
  if (idP > 0) return;

  doodle::Assclass table{};

  auto db = coreSql::getCoreSql().getConnection();
  auto install =
      sqlpp::insert_into(table).columns(table.assName, table.assdepId);
  install.values.add(table.assName = name, table.assdepId = p_assDep_id);
  idP = db->insert(install);

  if (idP == 0) {
    throw std::runtime_error("not insert assclass");
  }
  if (p_ptr_znch) p_ptr_znch->insert();
  coreDataManager::get().setAssClassL(shared_from_this());
}

void assClass::updateSQL() {
  //转发只更新中文名称
  p_ptr_znch->updateSQL();
}

void assClass::deleteSQL() {
  if (p_ptr_znch) p_ptr_znch->deleteSQL();
  doodle::Assclass table{};

  auto db = coreSql::getCoreSql().getConnection();
  db->remove(sqlpp::remove_from(table).where(table.id == idP));
  //我们需要在总的设置里面删除刚刚的指针
  auto list = coreDataManager::get().getAssClassL();

  list.erase(std::remove_if(list.begin(), list.end(),
                            [=](assClassPtr &info) -> bool {
                              return info == this->shared_from_this();
                            }),
             list.end());

  coreDataManager::get().setAssClassL(list);
}

assClassPtrList assClass::getAll(const assDepPtr &ass_dep_ptr) {
  assClassPtrList list;

  doodle::Znch znNa{};
  doodle::Assclass table{};  //.left_outer_join(znNa)
  auto db = coreSql::getCoreSql().getConnection();
  // sqlpp::all_of(table),sqlpp::all_of(znNa)

  for (auto &&row :
       db->run(sqlpp::select(table.id, table.assdepId, table.assName,
                             znNa.localname, znNa.id.as(znID))
                   .where(table.assdepId == ass_dep_ptr->getIdP())
                   .from(table.left_outer_join(znNa).on(table.id ==
                                                        znNa.assClassId))
                   .flags(sqlpp::distinct)
                   .order_by(table.assName.asc()))) {
    auto assclass = std::make_shared<assClass>();

    assclass->name = row.assName;
    assclass->idP = row.id;
    assclass->setAssDep(ass_dep_ptr);
    if (row.localname._is_valid) {
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

std::string assClass::getAssClass() const { return name; }
std::string assClass::getAssClass(const bool &isZNCH) {
  std::string str;
  if (isZNCH) {
    if (!p_ptr_znch) {
      p_ptr_znch = std::make_shared<znchName>(this);
    }
    str = p_ptr_znch->getName();
  } else {
    str = name;
  }

  return str;
}

CORE_NAMESPACE_E
