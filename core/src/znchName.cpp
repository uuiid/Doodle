/*
 * @Author: your name
 * @Date: 2020-10-19 13:26:31
 * @LastEditTime: 2020-11-26 17:56:15
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\znchName.cpp
 */
#include "znchName.h"

#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>

#include <stdexcept>

#include "Logger.h"
#include "assClass.h"
#include "coreOrm/znch_sqlOrm.h"
#include "coreset.h"
#include "coresql.h"
#include "src/convert.h"

CORE_NAMESPACE_S

znchName::znchName(assClass *at_)
    : coresqldata(),
      nameEN(),
      nameZNCH(),
      con(dopinyin::convertPtr()),
      p_ptr_assType(at_) {}

void znchName::setName(const std::string &name_) { nameEN = name_; }

void znchName::insert() {
  if (p_ptr_assType == nullptr) return;
  if (idP > 0) return;

  doodle::Znch table{};

  auto db = coreSql::getCoreSql().getConnection();
  auto install =
      sqlpp::insert_into(table).columns(table.localname, table.assClassId);
  install.values.add(table.localname = nameZNCH,
                     table.assClassId = p_ptr_assType->getIdP());

  idP = db->insert(install);
  if (idP == 0) {
    DOODLE_LOG_INFO << nameZNCH.c_str();
    DOODLE_LOG_WARN << "not install znch";
    throw std::runtime_error("not install znch");
  }
}

void znchName::select() {}

void znchName::updateSQL() {
  if (idP <= 0) return;

  doodle::Znch table{};
  auto db = coreSql::getCoreSql().getConnection();
  db->update(sqlpp::update(table)
                 .set(table.localname = nameZNCH)
                 .where(table.id == idP));
}

void znchName::deleteSQL() {
  doodle::Znch table{};
  auto db = coreSql::getCoreSql().getConnection();
  db->remove(sqlpp::remove_from(table).where(table.id == idP));
}

void znchName::setName(const std::string &name_, const bool &isZNCH) {
  if (!con) con = dopinyin::convertPtr(new dopinyin::convert);
  nameZNCH = name_;
  nameEN = con->toEn(name_);
}

std::string znchName::getName() const {
  if (!nameZNCH.empty())
    return nameZNCH;
  else if (!nameEN.empty())
    return nameEN;
  else
    return "";
}
std::string znchName::pinyin() const { return nameEN; }
CORE_NAMESPACE_E