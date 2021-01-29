#include "assClass.h"
#include <corelib/core/coresql.h>

#include <corelib/assets/assdepartment.h>

#include <corelib/assets/znchName.h>

#include <loggerlib/Logger.h>

#include <stdexcept>
#include <corelib/coreOrm/assclass_sqlOrm.h>
#include <corelib/coreOrm/znch_sqlOrm.h>
#include <corelib/coreOrm/basefile_sqlOrm.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

//反射使用
#include <rttr/registration>
#include <memory>

//注册sql库使用的外键

DOODLE_NAMESPACE_S
SQLPP_ALIAS_PROVIDER(znID)

RTTR_REGISTRATION {
  rttr::registration::class_<assClass>(DOCORE_RTTE_CLASS(assClass))
      .constructor<>()(rttr::policy::ctor::as_std_shared_ptr);
}

DOODLE_INSRANCE_CPP(assClass);
assClass::assClass()
    : coresqldata(),
      std::enable_shared_from_this<assClass>(),
      name(),
      p_assDep_id(-1),
      p_ass_dep_ptr_(),
      p_ptr_znch() {
  p_instance.insert(this);
}

assClass::~assClass() {
  p_instance.erase(this);
}

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
}

assClassPtrList assClass::getAll(const assDepPtr &ass_dep_ptr) {
  assClassPtrList list;

  doodle::Znch znNa{};
  doodle::Assclass table{};  //.left_outer_join(znNa)
  doodle::Basefile base_file{};
  auto db = coreSql::getCoreSql().getConnection();

  auto quert =
      sqlpp::select(table.id, table.assdepId, table.assName,
                    znNa.localname, znNa.id.as(znID))
          .where(table.assdepId == ass_dep_ptr->getIdP())
          .from(table.left_outer_join(znNa)
                    .on(table.id == znNa.assClassId))
          .flags(sqlpp::distinct)
          .order_by(table.assName.asc());

  /*
      sqlpp::select(table.id, table.assdepId, table.assName,
                    znNa.localname, znNa.id.as(znID), base_file.filetime.as(basefile_time))
          .where(table.assdepId == ass_dep_ptr->getIdP())
          .from(table.left_outer_join(znNa)
                    .on(table.id == znNa.assClassId)  //
                    .left_outer_join(base_file)
                    .on(table.id == base_file.assClassId))
          .flags(sqlpp::distinct)
          .order_by(table.assName.asc());
  */
  for (auto &&row :
       db->run(quert)) {
    auto assclass = std::make_shared<assClass>();

    assclass->name = row.assName;
    assclass->idP  = row.id;
    assclass->setAssDep(ass_dep_ptr);
    if (row.localname._is_valid) {
      assclass->p_ptr_znch           = std::make_shared<znchName>(assclass.get());
      assclass->p_ptr_znch->nameZNCH = row.localname;
      assclass->p_ptr_znch->idP      = row.znID;
      assclass->p_ptr_znch->nameEN   = row.assName;
    }

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
  p_assDep_id    = value->getIdP();
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
const std::unordered_set<assClass *> assClass::Instances() {
  return p_instance;
}
DOODLE_NAMESPACE_E
