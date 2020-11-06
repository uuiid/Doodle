#include "shotfilesqlinfo.h"

#include "coreset.h"
#include "coresql.h"
#include "episodes.h"
#include "shot.h"
#include "shotClass.h"
#include "shottype.h"

#include "Logger.h"

#include <iostream>

CORE_NAMESPACE_S

shotFileSqlInfo::shotFileSqlInfo()
    : fileSqlInfo(),
      p_shot_id(-1),
      p_eps_id(-1),
      p_shCla_id(-1),
      p_shTy_id(-1),
      p_ptr_eps(),
      p_ptr_shot(),
      p_ptr_shTy(),
      p_ptr_shcla() {

}

void shotFileSqlInfo::select(const qint64 &ID_) {
//  sql::SelectModel sel_;
//  sel_.select("id", "file", "fileSuffixes", "user", "version",
//              "_file_path_", "infor", "filestate",
//              "p_eps_id", "p_shot_id", "p_assDep_id", "ass_type_id");
//
//  sel_.from(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
//  sel_.where(sql::column("id") == ID_);
//
//  sqlQuertPtr query = coreSql::getCoreSql().getquery();
//  if (!query->exec(QString::fromStdString(sel_.str())))
//    throw std::runtime_error(query->lastError().text().toStdString());
//  if (query->next()) {
//    idP = query->value(0).toInt();
//    fileP = query->value(1).toString();
//    fileSuffixesP = query->value(2).toString();
//    userP = query->value(3).toString();
//    versionP = query->value(4).toInt();
//    filepathP = query->value(5).toString();
//    infoP = query->value(6).toByteArray();
//    fileStateP = query->value(7).toString();
//
//    if (!query->value(8).isNull())
//      p_eps_id = query->value(8).toInt();
//
//    if (!query->value(9).isNull())
//      p_shot_id = query->value(9).toInt();
//
//    if (!query->value(10).isNull())
//      p_shCla_id = query->value(10).toInt();
//
//    if (!query->value(11).isNull())
//      p_shTy_id = query->value(11).toInt();
//  }
}

void shotFileSqlInfo::insert() {
//  sql::InsertModel ins_;
//  if (idP < 0) {
//    sqlQuertPtr query = coreSql::getCoreSql().getquery();
//    ins_.insert("file", fileP.toStdString());
//    ins_.insert("fileSuffixes", fileSuffixesP.toStdString());
//    ins_.insert("user", userP.toStdString());
//    ins_.insert("version", versionP);
//    ins_.insert("_file_path_", filepathP.toStdString());
//
//    if (!infoP.isEmpty())
//      ins_.insert("infor", infoP.toStdString());
//
//    if (!fileStateP.isEmpty())
//      ins_.insert("filestate", fileStateP.toStdString());
//
//    if (p_eps_id > 0)
//      ins_.insert("p_eps_id", p_eps_id);
//    if (p_shot_id > 0)
//      ins_.insert("p_shot_id", p_shot_id);
//    if (p_shCla_id > 0)
//      ins_.insert("p_assDep_id", p_shCla_id);
//    if (p_shTy_id > 0)
//      ins_.insert("ass_type_id", p_shTy_id);
//
//    ins_.into(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
//
//    DOODLE_LOG_DEBUG << QString::fromStdString(ins_.str());
//    if (!query->exec(QString::fromStdString(ins_.str())))
//      throw std::runtime_error(query->lastError().text().toStdString());
//    getInsertID(query);
//    DOODLE_LOG_INFO << " \n 插入数据库id :" << idP;
//    query->finish();
//  }
}

void shotFileSqlInfo::updateSQL() {
//  sql::UpdateModel upd_;
//  upd_.update(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
//  upd_.set("filestate", fileSuffixesP.toStdString());
//  upd_.set("infor", infoP.toStdString());
//
//  if ((p_eps_id >= 0) && (p_eps_id != p_ptr_eps.lock()->getIdP()))
//    upd_.set("p_eps_id", p_eps_id);
//  if ((p_shot_id >= 0) && (p_shot_id != p_ptr_shot.lock()->getIdP()))
//    upd_.set("p_shot_id", p_shot_id);
//  if ((p_shCla_id >= 0) && (p_shCla_id != p_ptr_shcla.lock()->getIdP()))
//    upd_.set("p_assDep_id", p_shCla_id);
//  if ((p_shTy_id >= 0) && (p_shTy_id != p_ptr_shTy.lock()->getIdP()))
//    upd_.set("ass_type_id", p_shTy_id);
//
//  upd_.where(sql::column("id") == idP);
//
//  sqlQuertPtr query = coreSql::getCoreSql().getquery();
//  if (!query->exec(QString::fromStdString(upd_.str())))
//    throw std::runtime_error("not updata fileinfo");
//  query->finish();
}

void shotFileSqlInfo::deleteSQL() {
}
template<typename T>
void shotFileSqlInfo::batchQuerySelect(T &row) {
//  shotInfoPtrList listShot;
//  while (query->next()) {
//    shotInfoPtr ptr(new shotFileSqlInfo);
//
//    ptr->idP = query->value(0).toInt();
//    ptr->fileP = query->value(1).toString();
//    ptr->fileSuffixesP = query->value(2).toString();
//    ptr->userP = query->value(3).toString();
//    ptr->versionP = query->value(4).toInt();
//    ptr->filepathP = query->value(5).toString();
//    ptr->infoP = query->value(6).toByteArray();
//    ptr->fileStateP = query->value(7).toString();
//
//    if (!query->value(8).isNull())
//      ptr->p_eps_id = query->value(8).toInt();
//
//    if (!query->value(9).isNull())
//      ptr->p_shot_id = query->value(9).toInt();
//
//    if (!query->value(10).isNull())
//      ptr->p_shCla_id = query->value(10).toInt();
//
//    if (!query->value(11).isNull())
//      ptr->p_shTy_id = query->value(11).toInt();
//
//    listShot.append(ptr);
//  }
//  return listShot;
}

shotInfoPtrList shotFileSqlInfo::getAll(const episodesPtr &EP_) {
//  sql::SelectModel sel_;
//  sel_.select("id", "file", "fileSuffixes", "user", "version",
//              "_file_path_", "infor", "filestate",
//              "p_eps_id", "p_shot_id", "p_assDep_id", "ass_type_id");
//
//  sel_.from(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
//  sel_.where(sql::column("p_eps_id") == EP_->getIdP());
//
//  sqlQuertPtr query = coreSql::getCoreSql().getquery();
//  if (!query->exec(QString::fromStdString(sel_.str())))
//    throw std::runtime_error("not exe query");
//
//  shotInfoPtrList listInfo = batchQuerySelect(query);
//  for (auto &x : listInfo) {
//    x->setEpisdes(EP_);
//  }
//  return listInfo;
}

shotInfoPtrList shotFileSqlInfo::getAll(const shotPtr &sh_) {
//  sql::SelectModel sel_;
//  sel_.select("id", "file", "fileSuffixes", "user", "version",
//              "_file_path_", "infor", "filestate",
//              "p_eps_id", "p_shot_id", "p_assDep_id", "ass_type_id");
//
//  sel_.from(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
//  sel_.where(sql::column("p_shot_id") == sh_->getIdP());
//
//  sqlQuertPtr query = coreSql::getCoreSql().getquery();
//  if (!query->exec(QString::fromStdString(sel_.str())))
//    throw std::runtime_error("not exe query");
//
//  shotInfoPtrList listInfo = batchQuerySelect(query);
//  for (auto &x : listInfo) {
//    x->setShot(sh_);
//  }
//  return listInfo;
}

shotInfoPtrList shotFileSqlInfo::getAll(const shotClassPtr &fc_) {
//  sql::SelectModel sel_;
//  sel_.select("id", "file", "fileSuffixes", "user", "version",
//              "_file_path_", "infor", "filestate",
//              "p_eps_id", "p_shot_id", "p_assDep_id", "ass_type_id");
//
//  sel_.from(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
//  sel_.where(sql::column("p_assDep_id") == fc_->getIdP());
//
//  sqlQuertPtr query = coreSql::getCoreSql().getquery();
//  if (!query->exec(QString::fromStdString(sel_.str())))
//    throw std::runtime_error("not exe query");
//
//  shotInfoPtrList listInfo = batchQuerySelect(query);
//  for (auto &x : listInfo) {
//    x->setAssDep(fc_);
//  }
//  return listInfo;
}

shotInfoPtrList shotFileSqlInfo::getAll(const shotTypePtr &ft_) {
//  sql::SelectModel sel_;
//  sel_.select("id", "file", "fileSuffixes", "user", "version",
//              "_file_path_", "infor", "filestate",
//              "p_eps_id", "p_shot_id", "p_assDep_id", "ass_type_id");
//
//  sel_.from(QString("%1.basefile").arg(coreSet::getCoreSet().getProjectname()).toStdString());
//  sel_.order_by("filetime DESC");
//  sel_.where(sql::column("ass_type_id") == ft_->getIdP());
//
//  sqlQuertPtr query = coreSql::getCoreSql().getquery();
//  if (!query->exec(QString::fromStdString(sel_.str())))
//    throw std::runtime_error("not exe query");
//
//  shotInfoPtrList listInfo = batchQuerySelect(query);
//  for (auto &x : listInfo) {
//    x->setAssClass(ft_);
//  }
//  return listInfo;
}

dpath shotFileSqlInfo::generatePath(const std::string &programFolder) {
  QString str("%1/%2/%3/%4/%5/%6");
  coreSet &set = coreSet::getCoreSet();
  //第一次格式化添加根路径
  str = str.arg(set.getShotRoot().absolutePath());
  //第二次格式化添加集数字符串
  episodesPtr ep_ = getEpisdes();
  if (ep_ != nullptr)
    str = str.arg(ep_->getEpisdes_str());
  else
    str = str.arg(QString());

  //第三次格式化添加镜头字符串
  shotPtr sh_ = getShot();
  if (sh_ != nullptr)
    str = str.arg(sh_->getShotAndAb_str());
  else
    str = str.arg(QString());

  //第四次格式化添加程序文件夹
  str = str.arg(programFolder);

  //第五次格式化添加部门文件夹
  if (getShotclass())
    str = str.arg(coreSet::getCoreSet().getDepartment());
  else
    str = str.arg(QString());

  //第六次格式化添加类别文件夹
  shotTypePtr ft_ = getShotType();
  if (ft_ != nullptr)
    str = str.arg(ft_->getType());
  else
    str = str.arg(QString());

  return QDir::cleanPath(str);
}

dpath shotFileSqlInfo::generatePath(const dstring &programFolder, const dstring &suffixes) {
  QString str("%1/%2");
  str = str.arg(generatePath(programFolder));

  str = str.arg(generateFileName(suffixes));

  return str;
}

dpath shotFileSqlInfo::generatePath(const dstring &programFolder, const dstring &suffixes, const dstring &prefix) {
  QString str("%1/%2");
  str = str.arg(generatePath(programFolder));

  str = str.arg(generateFileName(suffixes, prefix));

  return str;
}

dstring shotFileSqlInfo::generateFileName(const dstring &suffixes) {

  QString name("shot_%1_%2_%3_%4_v%5_%6.%7");
  //第一次 格式化添加 集数
  episodesPtr ep_ = getEpisdes();
  if (ep_ != nullptr)
    name = name.arg(ep_->getEpisdes_str());
  else
    name = name.arg(QString());

  //第二次格式化添加 镜头号
  shotPtr sh_ = getShot();
  if (sh_ != nullptr)
    name = name.arg(sh_->getShotAndAb_str());
  else
    name = name.arg(QString());

  //第三次格式化添加 fileclass
  if (getShotclass())
    name = name.arg(coreSet::getCoreSet().getDepartment());
  else
    name = name.arg(QString());

  //第四次格式化添加 shotType
  shotTypePtr ft_ = getShotType();
  if (ft_ != nullptr)
    name = name.arg(ft_->getType());
  else
    name = name.arg(QString());

  name = name.arg(versionP, 4, 10, QLatin1Char('0'));
  name = name.arg(coreSet::getCoreSet().getUser_en());
  name = name.arg(suffixes);

  return name;
}

dstring shotFileSqlInfo::generateFileName(const dstring &suffixes, const dstring &prefix) {
  QString name("%1_%2");
  name = name.arg(prefix);
  name = name.arg(generateFileName(suffixes));
  return name;
}

episodesPtr shotFileSqlInfo::getEpisdes() {
  if (p_ptr_eps) {
    return p_ptr_eps;
  } else if (p_eps_id >= 0) {
    episodesPtr p_ = episodesPtr(new episodes);
    p_->select(p_eps_id);
    this->setEpisdes(p_);
    return p_;
  }
  return nullptr;
}

void shotFileSqlInfo::setEpisdes(const episodesPtrW &eps_) {
  if (!eps_)
    return;

  p_ptr_eps = eps_;
  p_eps_id = eps_.->getIdP();
}

shotPtr shotFileSqlInfo::getShot() {
  if (p_ptr_shot != nullptr) {
    return p_ptr_shot;
  } else if (p_shot_id >= 0) {
    shotPtr p_ = std::make_shared<shot>();
    p_->select(p_shot_id);
    p_ptr_shot = p_;
    return p_;
  }
  return nullptr;
}

void shotFileSqlInfo::setShot(const shotPtr &shot_) {
  if (!shot_)
    return;
  p_ptr_shot = shot_;
  p_shot_id = shot_->getIdP();

  setEpisdes(shot_->getEpisodes());
}

shotClassPtr shotFileSqlInfo::getShotclass() {
  if (p_ptr_shcla)
    return p_ptr_shcla;
  else if (p_shCla_id >= 0) {
    shotClassPtr p_ = std::make_shared<shotClass>();
    p_->select(p_shCla_id);
    p_ptr_shcla = p_;
    return p_;
  }
  return nullptr;
}

void shotFileSqlInfo::setShotClass(const shotClassPtr &value) {
  if (!value)
    return;
  p_shCla_id = value->getIdP();
  p_ptr_shcla = value;

  setShot(value->getShot());
}

shotTypePtr shotFileSqlInfo::getShotType() {
  if (p_ptr_shTy)
    return p_ptr_shTy;
  else if (p_shTy_id >= 0) {
    shotTypePtr p_ = std::make_shared<shotType>();
    p_->select(p_shTy_id);
    p_ptr_shTy = p_;
    return p_;
  }
  return nullptr;
}

void shotFileSqlInfo::setShotType(const shotTypePtr &fileType_) {
  if (!fileType_)
    return;
  p_shTy_id = fileType_->getIdP();
  p_ptr_shTy = fileType_;

  setShotClass(fileType_->getFileClass());
}
shotTypePtr shotFileSqlInfo::findFileType(const std::string &type_str) {
  //获得导出类别的约束
  auto fileTypelist = shotType::getAll(getShotclass());
  shotTypePtr k_fileType = nullptr;
  for (auto &&item:fileTypelist) {
    if (item->getType() == type_str) {
      k_fileType = item;
      break;
    }
  }
  if (!k_fileType) {
    k_fileType = std::make_shared<shotType>();
    k_fileType->setType(type_str);
    k_fileType->setFileClass(getShotclass());
    k_fileType->insert();
  }
  return k_fileType;
}

CORE_NAMESPACE_E
