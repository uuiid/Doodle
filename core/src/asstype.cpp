#include "sql_builder/sql.h"

#include "asstype.h"
#include "coreSet.h"
#include "fileclass.h"
#include "znchName.h"

#include <QVariant>
#include <QSqlError>
#include <QVector>

CORE_NAMESPACE_S

assType::assType() {
  name = QString();

  __file_class__ = -1;
  p_tprw_fileClass = nullptr;

  p_ptr_znch = nullptr;
}

void assType::select(const qint64 &ID_) {
  sql::SelectModel sel_;
  sel_.select("id", "file_name", "__file_class__");
  sel_.from(QString("%1.assclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());
  sel_.where(sql::column("id") == ID_);

  sqlQuertPtr query = coreSql::getCoreSql().getquery();

  if (!query->exec(QString::fromStdString(sel_.str())))
    throw std::runtime_error(query->lastError().text().toStdString());
  if (query->next()) {
    idP = query->value(0).toInt();
    name = query->value(1).toString();
    __file_class__ = query->value(2).toInt();
  }
}

void assType::insert() {
  sql::InsertModel ins_;
  if (idP < 0) {
    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    ins_.insert("file_name", name.toStdString());
    if (__file_class__ >= 0)
      ins_.insert("__file_class__", __file_class__);

    ins_.into(QString("%1.assclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());

    if (!query->exec(QString::fromStdString(ins_.str())))
      throw std::runtime_error(query->lastError().text().toStdString());
    getInsertID(query);

    query->finish();
  }
  if (p_ptr_znch)
    p_ptr_znch->insert();
}

void assType::updateSQL() {
  sql::UpdateModel upd_;

  upd_.update(QString("%1.assclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());

  upd_.set("__file_class__", __file_class__);

  upd_.where(sql::column("id") == idP);

  sqlQuertPtr query = coreSql::getCoreSql().getquery();
  if (!query->exec(QString::fromStdString(upd_.str())))
    throw std::runtime_error(query->lastError().text().toStdString());
  query->finish();
}

void assType::deleteSQL() {
}

assTypePtrList assType::batchQuerySelect(sqlQuertPtr &query) {
  assTypePtrList listassType;
  while (query->next()) {
    assTypePtr p_(new assType);
    p_->idP = query->value(0).toInt();
    p_->name = query->value(1).toString();
    p_->__file_class__ = query->value(2).toInt();
    listassType.append(p_);
  }
  return listassType;
}

assTypePtrList assType::getAll(const fileClassPtr &fc_) {
  sql::SelectModel sel_;
  sel_.select("id", "file_name", "__file_class__");
  sel_.from(QString("%1.assclass").arg(coreSet::getCoreSet().getProjectname()).toStdString());
  sel_.order_by("file_name");
  sel_.where(sql::column("__file_class__") == fc_->getIdP());

  sqlQuertPtr query = coreSql::getCoreSql().getquery();

  if (!query->exec(QString::fromStdString(sel_.str())))
    throw std::runtime_error(query->lastError().text().toStdString());

  assTypePtrList listassType = batchQuerySelect(query);
  for (auto &x : listassType) {
    x->setFileClass(fc_);
  }
  return listassType;
}

fileClassPtr assType::getFileClass() {
  if (p_tprw_fileClass != nullptr) {
    return p_tprw_fileClass;
  } else {
    fileClassPtr p_ = fileClassPtr(new fileClass);
    p_->select(__file_class__);
    this->p_tprw_fileClass = p_;
    return p_;
  }
}

void assType::setFileClass(const fileClassPtrW &value) {
  p_tprw_fileClass = value;
  __file_class__ = value.lock()->getIdP();
}

void assType::setAssType(const QString &value) {
  if (!p_ptr_znch) {
    p_ptr_znch = znchNamePtr(new znchName(this));
  }

  p_ptr_znch->setName(value, true);
  name = p_ptr_znch->pinyin();
}

void assType::setAssType(const QString &value, const bool &isZNCH) {
  setAssType(value);
}

QString assType::getAssType() const {
  return name;
}

QString assType::getAssType(const bool &isZNCH) {
  QString str;
  if (!p_ptr_znch) {
    p_ptr_znch = znchNamePtr(new znchName(this));
    p_ptr_znch->select();
  }
  str = p_ptr_znch->getName();
  return str;
}

CORE_NAMESPACE_E
