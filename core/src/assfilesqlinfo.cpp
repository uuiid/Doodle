#include "assfilesqlinfo.h"

#include "coreset.h"
#include "coresql.h"

#include "assdepartment.h"
#include "assClass.h"
#include "assType.h"

#include "Logger.h"

#include "coreOrm/basefile_sqlOrm.h"
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

#include <QVariant>
#include <QSqlError>

#include <iostream>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <filesystem>
CORE_NAMESPACE_S

assFileSqlInfo::assFileSqlInfo() :
    p_type_ptr_(),
    p_class_ptr_(),
    p_dep_ptr_(),
    ass_type_id(-1),
    ass_class_id(-1) {

}

void assFileSqlInfo::select(qint64 &ID_) {
  doodle::Basefile tab;

  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row:db->run(
      sqlpp::select(sqlpp::all_of(tab))
          .where(tab.id == ID_)
  )) {
    batchSetAttr(row);
  }
}

void assFileSqlInfo::insert() {
  if (idP > 0) return;
  doodle::Basefile tab;

  auto db = coreSql::getCoreSql().getConnection();
  auto install = sqlpp::insert_into(tab);
  install.set(
      tab.file = fileP,
      tab.fileSuffixes = fileSuffixesP,
      tab.user = userP,
      tab.version = versionP,
      tab.FilePath_ = filepathP,
      tab.filestate = sqlpp::value_or_null(fileStateP)
  );
  if (!infoP.empty())
    install.set(tab.infor = infoP);
  if (ass_class_id > 0)
    install.set(tab.assClassId = ass_class_id);
  if (ass_type_id > 0)
    install.set(tab.assTypeId = ass_type_id);
  idP = db->insert(install);
}

void assFileSqlInfo::updateSQL() {
  if (idP < 0) return;
  doodle::Basefile tab;

  auto db = coreSql::getCoreSql().getConnection();
  auto updata = sqlpp::update(tab);
  updata.set(
      tab.infor = infoP,
      tab.filestate = fileStateP
  );
  db->update(updata);
}

void assFileSqlInfo::deleteSQL() {
}

assInfoPtrList assFileSqlInfo::getAll(const assDepPtr &fc_) {
  doodle::Basefile tab;
  assInfoPtrList list;

  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row :db->run(
      sqlpp::select(sqlpp::all_of(tab))
          .where(tab.assTypeId == fc_->getIdP())
  )) {
    auto assInfo = std::make_shared<assFileSqlInfo>();
    assInfo->batchSetAttr(row);
    assInfo->setAssDep(fc_);
    list.push_back(assInfo);
  }
  return list;
}

assInfoPtrList assFileSqlInfo::getAll(const assClassPtr &AT_) {
  doodle::Basefile tab;
  assInfoPtrList list;

  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row :db->run(sqlpp::select(sqlpp::all_of(tab))
                               .where(tab.assTypeId == AT_->getIdP())
  )) {
    auto assInfo = std::make_shared<assFileSqlInfo>();
    assInfo->batchSetAttr(row);
    assInfo->setAssClass(AT_);
    list.push_back(assInfo);
  }
  return list;
}

assInfoPtrList assFileSqlInfo::getAll(const assTypePtr &ft_) {
  doodle::Basefile tab;
  assInfoPtrList list;

  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row :db->run(sqlpp::select(sqlpp::all_of(tab))
                               .where(tab.assTypeId == ft_->getIdP())
  )) {
    auto assInfo = std::make_shared<assFileSqlInfo>();
    assInfo->batchSetAttr(row);
    assInfo->setAssType(ft_);
    list.push_back(assInfo);
  }
  return list;
}
dpath assFileSqlInfo::generatePath(const std::string &programFolder) {
//  QString path("%1/%2/%3/%4/%5");

  //第一次 格式化添加根目录
  dpath path = coreSet::getCoreSet().getAssRoot().absolutePath().toStdString();

  //第二次添加类型
  auto dep = getAssDep();
  if (dep)
    path = path / dep->getAssDep();

  //第三次格式化添加  ass_type
  auto as_cls = getAssClass();
  if (as_cls)
    path = path / as_cls->getAssClass();

  //第四次次格式化程序文件夹
  path = path / programFolder;

  //第五次添加fileType
  auto as_ty = getAssType();
  if (as_ty) {
    path = path / as_ty->getType();
  }

  return path;
}

dpath assFileSqlInfo::generatePath(const dstring &programFolder, const dstring &suffixes) {
  return generatePath(programFolder) / generateFileName(suffixes);
}

dpath assFileSqlInfo::generatePath(const dstring &programFolder, const dstring &suffixes, const dstring &prefix) {
  return generatePath(programFolder) / generateFileName(suffixes, prefix);
}

dstring assFileSqlInfo::generateFileName(const dstring &suffixes) {

  boost::format format("%1%%2%.%3%");
  auto as_cl = getAssClass();
  if (as_cl)
    format % as_cl->getAssClass();
  else
    format % "";

  auto as_ty = getAssType();
  if (as_ty) {
    if (as_ty->getType() == "rig")
      format % as_ty->getType();
    else
      format % "";
  } else
    format % "";

  return format.str();
}

dstring assFileSqlInfo::generateFileName(const dstring &suffixes, const dstring &prefix) {
  boost::format str("%1%_%2%");
  str % prefix % generateFileName(suffixes);
  return str.str();
}

assDepPtr assFileSqlInfo::getAssDep() {
  return p_dep_ptr_;
}

void assFileSqlInfo::setAssDep(const assDepPtr &ass_dep_) {
  if (!ass_dep_)
    return;
  p_dep_ptr_ = ass_dep_;
}

const assClassPtr &assFileSqlInfo::getAssClass() {
  if (!p_class_ptr_) {
    p_class_ptr_ = std::make_shared<assClass>();
    p_class_ptr_->select(ass_class_id);
  }
  return p_class_ptr_;
}

void assFileSqlInfo::setAssClass(const assClassPtr &ass_type_) {
  if (!ass_type_)
    return;
  p_type_ptr_ = ass_type_;
  ass_type_id = ass_type_->getIdP();

  setAssDep(ass_type_->getAssDep());
}

const assTypePtr &assFileSqlInfo::getAssType() {
  if (!p_type_ptr_) {
    p_type_ptr_ = std::make_shared<assClass>();
    p_type_ptr_->select(ass_type_id);
  }
  return p_type_ptr_;
}

void assFileSqlInfo::setAssType(const assTypePtr &ass_type_) {
  if (!ass_type_)
    return;
  p_class_ptr_ = ass_type_;
  ass_class_id = ass_type_->getIdP();

  setAssClass(ass_type_->getAssClassPtr());
}
template<typename T>
void assFileSqlInfo::batchSetAttr(const T &t) {
  idP = t.id;
  fileP = t.file;
  fileSuffixesP = t.fileSuffixes;
  userP = t.user;
  versionP = t.version;
  filepathP = t.FilePath_;
  infoP = t.infor;
  fileStateP = t.filestate;

  if (t.assClassId._is_valid)
    ass_class_id = t.assClassId;

  if (t.assTypeId._is_valid)
    ass_type_id = t.assTypeId;
}

CORE_NAMESPACE_E
