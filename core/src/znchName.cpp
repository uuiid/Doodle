#include "znchName.h"

#include "coreset.h"
#include "coresql.h"

#include "assClass.h"

#include "src/convert.h"
#include "Logger.h"
#include "coreOrm/znch_sqlOrm.h"
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

#include <stdexcept>

CORE_NAMESPACE_S

znchName::znchName(assClass *at_)
    : nameEN(),
      nameZNCH(),
      con(dopinyin::convertPtr()),
      p_ptr_assType() {

}

void znchName::setName(const std::string &name_) {
  nameEN = name_;
}

void znchName::insert() {
  if (p_ptr_assType == nullptr) return;
  if (idP > 0) return;

  doodle::Znch table;

  auto db = coreSql::getCoreSql().getConnection();
  idP = db->insert(sqlpp::insert_into(table).set(table.localname = nameZNCH,
                                                 table.assClassId = p_ptr_assType->getIdP()));
  if (idP == 0) {
    DOODLE_LOG_INFO << nameZNCH.c_str();
    DOODLE_LOG_WARN << "not install znch";
    throw std::runtime_error("not install znch");
  }
//  sql::InsertModel ins_;
//  if (idP < 0) {
//    sqlQuertPtr query = coreSql::getCoreSql().getquery();
//    ins_.insert("localname", nameZNCH.toStdString());
//
//    if (!p_ptr_assType)
//      throw std::runtime_error("not asstype ");
//    ins_.insert("ass_class_id", p_ptr_assType->getIdP());
//    //添加插入表
//    ins_.into(QString("%1.znch").arg(coreSet::getCoreSet().getProjectname()).toStdString());
//
//    if (!query->exec(QString::fromStdString(ins_.str())))
//      throw std::runtime_error(query->lastError().text().toStdString());
//
//    getInsertID(query);
//    query->finish();
//  }
}

void znchName::select() {
//  sql::SelectModel sel_;
//  sel_.select("id", "localname","ass_class_id");
//  sel_.from(QString("%1.znch").arg(coreSet::getCoreSet().getProjectname()).toStdString());
//  sel_.where(sql::column("ass_class_id") == p_ptr_assType->getIdP());
//
//  sqlQuertPtr query = coreSql::getCoreSql().getquery();
//
//  if (!query->exec(QString::fromStdString(sel_.str())))
//    throw std::runtime_error(query->lastError().text().toStdString());
//
//  if (query->next()) {
//    idP = query->value(0).toInt();
//    nameZNCH = query->value(1).toString();
//    assert(query->value(2).toUInt() == p_ptr_assType->getIdP());
//  } else {
//    idP = -1;
//    nameZNCH = p_ptr_assType->getAssClass();
//  }
}

void znchName::updateSQL() {
  if (idP <= 0) return;

  doodle::Znch table;
  auto db = coreSql::getCoreSql().getConnection();
  db->update(sqlpp::update(table)
                 .set(table.localname = nameZNCH)
                 .where(table.id == idP));

//  sql::UpdateModel upd_;
//
//  upd_.update(QString("%1.znch").arg(coreSet::getCoreSet().getProjectname()).toStdString());
//
//  upd_.set("localname", nameZNCH.toStdString());
//
//  upd_.where(sql::column("id") == idP);
//
//  sqlQuertPtr query = coreSql::getCoreSql().getquery();
//
//  if (!query->exec(QString::fromStdString(upd_.str())))
//    throw std::runtime_error(query->lastError().text().toStdString());
//  query->finish();
}

void znchName::deleteSQL() {
}

void znchName::setName(const std::string &name_, const bool &isZNCH) {
  if (!con)
    con = dopinyin::convertPtr(new dopinyin::convert);
  nameZNCH = name_;
  nameEN = con->toEn(name_);
}

std::string znchName::getName() const {
  if (!nameZNCH.empty())
    return nameZNCH;
  else
    return nameEN;
}
std::string znchName::pinyin() const {
  return nameEN;
}
CORE_NAMESPACE_E