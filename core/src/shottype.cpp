#include "shottype.h"

#include "sql_builder/sql.h"
#include "coreset.h"
#include "shotClass.h"
#include "assClass.h"
#include "episodes.h"
#include "shot.h"

#include "Logger.h"

#include <QVariant>
#include <QSqlError>
#include <QVector>

#include <iostream>

CORE_NAMESPACE_S

shotType::shotType() {
  p_Str_Type = QString();

  p_fileClass = nullptr;
  p_assType = nullptr;
  p_episdes = nullptr;
  p_shot = nullptr;

  p_shotClass_id = -1;
  __ass_class__ = -1;
  p_eps_id = -1;
  p_shot_id = -1;
}

void shotType::select(const qint64 &ID_) {
  sql::SelectModel sel_;
  sel_.select("id", "file_type",
              "p_assDep_id", "ass_class_id", "p_eps_id", "p_shot_id");

  sel_.from(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());
  sel_.where(sql::column("id") == ID_);

  sqlQuertPtr query = coreSql::getCoreSql().getquery();
  if (!query->exec(QString::fromStdString(sel_.str())))
    return;
  if (query->next()) {
    idP = query->value(0).toInt();
    p_Str_Type = query->value(1).toString();

    if (!query->value(2).isNull())
      p_shotClass_id = query->value(2).toInt();

    if (!query->value(3).isNull())
      __ass_class__ = query->value(3).toInt();

    if (!query->value(4).isNull())
      p_eps_id = query->value(4).toInt();

    if (!query->value(5).isNull())
      p_shot_id = query->value(5).toInt();
  }
}

void shotType::insert() {
  sql::InsertModel ins_;
  if (idP < 0) {
    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    ins_.insert("file_type", p_Str_Type.toStdString());

    if (p_shotClass_id > 0)
      ins_.insert("p_assDep_id", p_shotClass_id);
    if (__ass_class__ > 0)
      ins_.insert("ass_class_id", __ass_class__);
    if (p_eps_id > 0)
      ins_.insert("p_eps_id", p_eps_id);
    if (p_shot_id > 0)
      ins_.insert("p_shot_id", p_shot_id);

    ins_.into(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());

    if (!query->exec(QString::fromStdString(ins_.str())))
      throw std::runtime_error(query->lastError().text().toStdString());
    getInsertID(query);

    query->finish();
  }
}

void shotType::updateSQL() {
  sql::UpdateModel upd_;
  upd_.update(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());
  if (p_eps_id > 0)
    upd_.set("p_eps_id", p_eps_id);

  if (p_shot_id > 0)
    upd_.set("p_shot_id", p_shot_id);

  if (p_shotClass_id > 0)
    upd_.set("p_assDep_id", p_shotClass_id);

  if (__ass_class__ > 0)
    upd_.set("ass_class_id", __ass_class__);

  upd_.where(sql::column("id") == idP);

  sqlQuertPtr query = coreSql::getCoreSql().getquery();
  if (!query->exec(QString::fromStdString(upd_.str()))) {
    throw std::runtime_error(query->lastError().text().toStdString());
  }
  query->finish();
}

void shotType::deleteSQL() {
}

shotTypePtrList shotType::batchQuerySelect(sqlQuertPtr &query) {
  shotTypePtrList list_;
  while (query->next()) {
    shotTypePtr p_ = shotTypePtr(new shotType);
    p_->idP = query->value(0).toInt();
    p_->p_Str_Type = query->value(1).toString();
    if (!query->value(2).isNull())
      p_->p_shotClass_id = query->value(2).toInt();

    if (!query->value(3).isNull())
      p_->__ass_class__ = query->value(3).toInt();

    if (!query->value(4).isNull())
      p_->p_eps_id = query->value(4).toInt();

    if (!query->value(5).isNull())
      p_->p_shot_id = query->value(5).toInt();

    list_.append(p_);
  }
  return list_;
}

shotTypePtrList shotType::getAll(const shotClassPtr &fc_) {
  sql::SelectModel sel_;
  sel_.select("id", "file_type",
              "p_assDep_id", "ass_class_id", "p_eps_id", "p_shot_id");

  sel_.from(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());
  sel_.where(sql::column("p_assDep_id") == fc_->getIdP());

  sqlQuertPtr query = coreSql::getCoreSql().getquery();
  if (!query->exec(QString::fromStdString(sel_.str()))) {
    DOODLE_LOG_WARN << query->lastError().text();
    throw std::runtime_error(query->lastError().text().toStdString());
  }

  shotTypePtrList listfileTypes = batchQuerySelect(query);

  for (auto &x : listfileTypes) {
    x->setAssDep(fc_);
  }
  return listfileTypes;
}

shotTypePtrList shotType::getAll(const assClassPtr &AT_) {
  sql::SelectModel sel_;
  sel_.select("id", "file_type",
              "p_assDep_id", "ass_class_id", "p_eps_id", "p_shot_id");

  sel_.from(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());

  sel_.where(sql::column("ass_class_id") == AT_->getIdP());

  sqlQuertPtr query = coreSql::getCoreSql().getquery();
  if (!query->exec(QString::fromStdString(sel_.str()))) {
    throw std::runtime_error(query->lastError().text().toStdString());
  }

  shotTypePtrList listfileTypes = batchQuerySelect(query);

  for (auto &x : listfileTypes) {
    x->setAssClass(AT_);
  }
  return listfileTypes;
}

shotTypePtrList shotType::getAll(const episodesPtr &EP_) {
  sql::SelectModel sel_;
  sel_.select("id", "file_type",
              "p_assDep_id", "ass_class_id", "p_eps_id", "p_shot_id");

  sel_.from(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());

  sel_.where(sql::column("p_eps_id") == EP_->getIdP());

  sqlQuertPtr query = coreSql::getCoreSql().getquery();
  if (!query->exec(QString::fromStdString(sel_.str()))) {
    throw std::runtime_error(query->lastError().text().toStdString());
  }

  shotTypePtrList listfileTypes = batchQuerySelect(query);

  for (auto &x : listfileTypes) {
    x->setEpisodes(EP_);
  }
  return listfileTypes;
}

shotTypePtrList shotType::getAll(const shotPtr &SH_) {
  sql::SelectModel sel_;
  sel_.select("id", "file_type",
              "p_assDep_id", "ass_class_id", "p_eps_id", "p_shot_id");

  sel_.from(QString("%1.filetype").arg(coreSet::getCoreSet().getProjectname()).toStdString());

  sel_.where(sql::column("p_shot_id") == SH_->getIdP());

  sqlQuertPtr query = coreSql::getCoreSql().getquery();
  if (!query->exec(QString::fromStdString(sel_.str()))) {
    throw std::runtime_error(query->lastError().text().toStdString());
  }

  shotTypePtrList listfileTypes = batchQuerySelect(query);

  for (auto &x : listfileTypes) {
    x->setShot(SH_);
  }
  return listfileTypes;
}

void shotType::setFileType(const QString &value) {
  p_Str_Type = value;
}

QString shotType::getFileType() const {
  return p_Str_Type;
}

void shotType::setFileClass(const fileClassPtrW &value) {
  if (!value)
    return;
  p_shotClass_id = value.lock()->getIdP();
  p_fileClass = value;

  if (p_shot_id >= 0) {
    setShot(value.lock()->getShot());
  }
}

shotClassPtr shotType::getFileClass() {
  if (p_fileClass)
    return p_fileClass;
  else if (p_shotClass_id > 0) {
    shotClassPtr p_ = shotClassPtr(new shotClass);
    p_->select(p_shotClass_id);
    p_fileClass = p_;
    return p_;
  } else {
    return nullptr;
  }
}
void shotType::setAssType(const assTypePtrW &assType_) {
  try {
    __ass_class__ = assType_.lock()->getIdP();
    p_assType = assType_;
  }
  catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
  setFileClass(assType_.lock()->getFileClass());
}

assClassPtr shotType::getAssType() {
  if (p_assType)
    return p_assType;
  else if (__ass_class__ >= 0) {
    assClassPtr p_ = assClassPtr(new assClass);
    p_->select(__ass_class__);
    p_assType = p_;
    return p_;
  } else {
    return nullptr;
  }
}

void shotType::setEpisodes(const episodesPtrW &value) {
  if (!value)
    return;
  p_eps_id = value.lock()->getIdP();
  p_episdes = value;
}

episodesPtr shotType::getEpisdes() {
  if (p_episdes) {
    return p_episdes;
  } else if (p_eps_id >= 0) {
    episodesPtr p_ = episodesPtr(new episodes);
    p_->select(p_eps_id);
    p_episdes = p_;
    return p_;
  } else {
    return nullptr;
  }
}

void shotType::setShot(const shotPtrW &shot_) {
  if (!shot_)
    return;
  p_shot_id = shot_.lock()->getIdP();
  p_shot = shot_;
  setEpisodes(shot_.lock()->getEpisodes());
}

shotPtr shotType::getShot() {
  if (p_shot) {
    return p_shot;
  } else if (p_shot_id >= 0) {
    shotPtr p_ = shotPtr(new shot);
    p_->select(p_shot_id);
    p_shot = p_;
    return p_shot;
  } else {
    return nullptr;
  }
}

CORE_NAMESPACE_E
