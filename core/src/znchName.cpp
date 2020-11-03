#include "znchName.h"

#include "sql_builder/sql.h"

#include "coreset.h"
#include "asstype.h"

#include "src/convert.h"

#include <QSqlError>
#include <stdexcept>

CORE_NAMESPACE_S

znchName::znchName(assType *at_) {
  nameZNCH = QString();
  con = dopinyin::convertPtr();

  p_ptr_assType = at_;
}

znchName::~znchName() {
}

void znchName::setName(const QString &name_) {
  nameEN = name_;
}

void znchName::insert() {
  sql::InsertModel ins_;
  if (idP < 0) {
    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    ins_.insert("localname", nameZNCH.toStdString());

    if (!p_ptr_assType)
      throw std::runtime_error("not asstype ");
    ins_.insert("__ass_class__", p_ptr_assType->getIdP());
    //添加插入表
    ins_.into(QString("%1.znch").arg(coreSet::getCoreSet().getProjectname()).toStdString());

    if (!query->exec(QString::fromStdString(ins_.str())))
      throw std::runtime_error(query->lastError().text().toStdString());

    getInsertID(query);
    query->finish();
  }
}

void znchName::select() {
  sql::SelectModel sel_;
  sel_.select("id", "localname","__ass_class__");
  sel_.from(QString("%1.znch").arg(coreSet::getCoreSet().getProjectname()).toStdString());
  sel_.where(sql::column("__ass_class__") == p_ptr_assType->getIdP());

  sqlQuertPtr query = coreSql::getCoreSql().getquery();

  if (!query->exec(QString::fromStdString(sel_.str())))
    throw std::runtime_error(query->lastError().text().toStdString());

  if (query->next()) {
    idP = query->value(0).toInt();
    nameZNCH = query->value(1).toString();
    assert(query->value(2).toUInt() == p_ptr_assType->getIdP());
  } else {
    idP = -1;
    nameZNCH = p_ptr_assType->getAssType();
  }
}

void znchName::updateSQL() {
  if (idP < 0) return;
  sql::UpdateModel upd_;

  upd_.update(QString("%1.znch").arg(coreSet::getCoreSet().getProjectname()).toStdString());

  upd_.set("localname", nameZNCH.toStdString());

  upd_.where(sql::column("id") == idP);

  sqlQuertPtr query = coreSql::getCoreSql().getquery();

  if (!query->exec(QString::fromStdString(upd_.str())))
    throw std::runtime_error(query->lastError().text().toStdString());
  query->finish();
}

void znchName::deleteSQL() {
}

void znchName::setName(const QString &name_, const bool &isZNCH) {
  if (!con)
    con = dopinyin::convertPtr(new dopinyin::convert);
  nameZNCH = name_;
  nameEN = con->toEn(name_);
}

QString znchName::getName() const {
  if (!nameZNCH.isNull())
    return nameZNCH;
  else
    return nameEN;
}
QString znchName::pinyin() const {
  return nameEN;
}
CORE_NAMESPACE_E