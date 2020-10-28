#include "filetype.h"

#include "sql_builder/sql.h"
#include "coreset.h"
#include "fileclass.h"
#include "asstype.h"
#include "episodes.h"
#include "shot.h"

#include "Logger.h"

#include <QVariant>
#include <QSqlError>
#include <QVector>

#include <iostream>

CORE_NAMESPACE_S

fileType::fileType() {
  p_Str_Type = QString();

  p_fileClass = nullptr;
  p_assType = nullptr;
  p_episdes = nullptr;
  p_shot = nullptr;

  __file_class__ = -1;
  __ass_class__ = -1;
  __episodes__ = -1;
  __shot__ = -1;
}

void fileType::select(const qint64 &ID_) {
  sql::SelectModel sel_;
  sel_.select("id", "file_type",
              "__file_class__", "__ass_class__", "__episodes__", "__shot__");

  sel_.from(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());
  sel_.where(sql::column("id") == ID_);

  sqlQuertPtr query = coreSql::getCoreSql().getquery();
  if (!query->exec(QString::fromStdString(sel_.str())))
    return;
  if (query->next()) {
    idP = query->value(0).toInt();
    p_Str_Type = query->value(1).toString();

    if (!query->value(2).isNull())
      __file_class__ = query->value(2).toInt();

    if (!query->value(3).isNull())
      __ass_class__ = query->value(3).toInt();

    if (!query->value(4).isNull())
      __episodes__ = query->value(4).toInt();

    if (!query->value(5).isNull())
      __shot__ = query->value(5).toInt();
  }
}

void fileType::insert() {
  sql::InsertModel ins_;
  if (idP < 0) {
    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    ins_.insert("file_type", p_Str_Type.toStdString());

    if (__file_class__ > 0)
      ins_.insert("__file_class__", __file_class__);
    if (__ass_class__ > 0)
      ins_.insert("__ass_class__", __ass_class__);
    if (__episodes__ > 0)
      ins_.insert("__episodes__", __episodes__);
    if (__shot__ > 0)
      ins_.insert("__shot__", __shot__);

    ins_.into(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());

    if (!query->exec(QString::fromStdString(ins_.str())))
      throw std::runtime_error(query->lastError().text().toStdString());
    getInsertID(query);

    query->finish();
  }
}

void fileType::updateSQL() {
  sql::UpdateModel upd_;
  upd_.update(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());
  if (__episodes__ > 0)
    upd_.set("__episodes__", __episodes__);

  if (__shot__ > 0)
    upd_.set("__shot__", __shot__);

  if (__file_class__ > 0)
    upd_.set("__file_class__", __file_class__);

  if (__ass_class__ > 0)
    upd_.set("__ass_class__", __ass_class__);

  upd_.where(sql::column("id") == idP);

  sqlQuertPtr query = coreSql::getCoreSql().getquery();
  if (!query->exec(QString::fromStdString(upd_.str()))) {
    throw std::runtime_error(query->lastError().text().toStdString());
  }
  query->finish();
}

void fileType::deleteSQL() {
}

fileTypePtrList fileType::batchQuerySelect(sqlQuertPtr &query) {
  fileTypePtrList list_;
  while (query->next()) {
    fileTypePtr p_ = fileTypePtr(new fileType);
    p_->idP = query->value(0).toInt();
    p_->p_Str_Type = query->value(1).toString();
    if (!query->value(2).isNull())
      p_->__file_class__ = query->value(2).toInt();

    if (!query->value(3).isNull())
      p_->__ass_class__ = query->value(3).toInt();

    if (!query->value(4).isNull())
      p_->__episodes__ = query->value(4).toInt();

    if (!query->value(5).isNull())
      p_->__shot__ = query->value(5).toInt();

    list_.append(p_);
  }
  return list_;
}

fileTypePtrList fileType::getAll(const fileClassPtr &fc_) {
  sql::SelectModel sel_;
  sel_.select("id", "file_type",
              "__file_class__", "__ass_class__", "__episodes__", "__shot__");

  sel_.from(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());
  sel_.where(sql::column("__file_class__") == fc_->getIdP());

  sqlQuertPtr query = coreSql::getCoreSql().getquery();
  if (!query->exec(QString::fromStdString(sel_.str()))) {
    DOODLE_LOG_WARN << query->lastError().text();
    throw std::runtime_error(query->lastError().text().toStdString());
  }

  fileTypePtrList listfileTypes = batchQuerySelect(query);

  for (auto &x : listfileTypes) {
    x->setFileClass(fc_);
  }
  return listfileTypes;
}

fileTypePtrList fileType::getAll(const assTypePtr &AT_) {
  sql::SelectModel sel_;
  sel_.select("id", "file_type",
              "__file_class__", "__ass_class__", "__episodes__", "__shot__");

  sel_.from(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());

  sel_.where(sql::column("__ass_class__") == AT_->getIdP());

  sqlQuertPtr query = coreSql::getCoreSql().getquery();
  if (!query->exec(QString::fromStdString(sel_.str()))) {
    throw std::runtime_error(query->lastError().text().toStdString());
  }

  fileTypePtrList listfileTypes = batchQuerySelect(query);

  for (auto &x : listfileTypes) {
    x->setAssType(AT_);
  }
  return listfileTypes;
}

fileTypePtrList fileType::getAll(const episodesPtr &EP_) {
  sql::SelectModel sel_;
  sel_.select("id", "file_type",
              "__file_class__", "__ass_class__", "__episodes__", "__shot__");

  sel_.from(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());

  sel_.where(sql::column("__episodes__") == EP_->getIdP());

  sqlQuertPtr query = coreSql::getCoreSql().getquery();
  if (!query->exec(QString::fromStdString(sel_.str()))) {
    throw std::runtime_error(query->lastError().text().toStdString());
  }

  fileTypePtrList listfileTypes = batchQuerySelect(query);

  for (auto &x : listfileTypes) {
    x->setEpisodes(EP_);
  }
  return listfileTypes;
}

fileTypePtrList fileType::getAll(const shotPtr &SH_) {
  sql::SelectModel sel_;
  sel_.select("id", "file_type",
              "__file_class__", "__ass_class__", "__episodes__", "__shot__");

  sel_.from(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());

  sel_.where(sql::column("__shot__") == SH_->getIdP());

  sqlQuertPtr query = coreSql::getCoreSql().getquery();
  if (!query->exec(QString::fromStdString(sel_.str()))) {
    throw std::runtime_error(query->lastError().text().toStdString());
  }

  fileTypePtrList listfileTypes = batchQuerySelect(query);

  for (auto &x : listfileTypes) {
    x->setShot(SH_);
  }
  return listfileTypes;
}

void fileType::setFileType(const QString &value) {
  p_Str_Type = value;
}

QString fileType::getFileType() const {
  return p_Str_Type;
}

void fileType::setFileClass(const fileClassPtrW &value) {
  if (!value)
    return;
  __file_class__ = value.lock()->getIdP();
  p_fileClass = value;

  if (__shot__ >= 0) {
    setShot(value.lock()->getShot());
  }
}

fileClassPtr fileType::getFileClass() {
  if (p_fileClass)
    return p_fileClass;
  else if (__file_class__ > 0) {
    fileClassPtr p_ = fileClassPtr(new fileClass);
    p_->select(__file_class__);
    p_fileClass = p_;
    return p_;
  } else {
    return nullptr;
  }
}
void fileType::setAssType(const assTypePtrW &assType_) {
  try {
    __ass_class__ = assType_.lock()->getIdP();
    p_assType = assType_;
  }
  catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
  setFileClass(assType_.lock()->getFileClass());
}

assTypePtr fileType::getAssType() {
  if (p_assType)
    return p_assType;
  else if (__ass_class__ >= 0) {
    assTypePtr p_ = assTypePtr(new assType);
    p_->select(__ass_class__);
    p_assType = p_;
    return p_;
  } else {
    return nullptr;
  }
}

void fileType::setEpisodes(const episodesPtrW &value) {
  if (!value)
    return;
  __episodes__ = value.lock()->getIdP();
  p_episdes = value;
}

episodesPtr fileType::getEpisdes() {
  if (p_episdes) {
    return p_episdes;
  } else if (__episodes__ >= 0) {
    episodesPtr p_ = episodesPtr(new episodes);
    p_->select(__episodes__);
    p_episdes = p_;
    return p_;
  } else {
    return nullptr;
  }
}

void fileType::setShot(const shotPtrW &shot_) {
  if (!shot_)
    return;
  __shot__ = shot_.lock()->getIdP();
  p_shot = shot_;
  setEpisodes(shot_.lock()->getEpisodes());
}

shotPtr fileType::getShot() {
  if (p_shot) {
    return p_shot;
  } else if (__shot__ >= 0) {
    shotPtr p_ = shotPtr(new shot);
    p_->select(__shot__);
    p_shot = p_;
    return p_shot;
  } else {
    return nullptr;
  }
}

CORE_NAMESPACE_E
