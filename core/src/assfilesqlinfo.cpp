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

#include <iostream>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <src/coreDataManager.h>
CORE_NAMESPACE_S

assFileSqlInfo::assFileSqlInfo() :
    fileSqlInfo(),
    std::enable_shared_from_this<assFileSqlInfo>(),
    p_type_ptr_(),
    p_class_ptr_(),
    p_dep_ptr_(),
    ass_type_id(-1),
    ass_class_id(-1) {

}

void assFileSqlInfo::select(qint64 &ID_) {
  doodle::Basefile tab{};

  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row:db->run(
      sqlpp::select(sqlpp::all_of(tab))
          .from(tab)
          .where(tab.id == ID_)
  )) {
    batchSetAttr(row);
  }
}

void assFileSqlInfo::insert() {
  if (idP > 0) return;
  doodle::Basefile tab{};

  auto db = coreSql::getCoreSql().getConnection();
  auto install = sqlpp::dynamic_insert_into(*db, tab).dynamic_set(
      tab.file = fileP,
      tab.fileSuffixes = fileSuffixesP,
      tab.user = userP,
      tab.version = versionP,
      tab.FilePath_ = filepathP,
      tab.filestate = sqlpp::value_or_null(fileStateP),
      tab.projectId = coreSet::getSet().projectName().first
  );
  if (!infoP.empty())
    install.insert_list.add(tab.infor = strList_tojson(infoP));
  if (ass_class_id > 0)
    install.insert_list.add(tab.assClassId = ass_class_id);
  if (ass_type_id > 0)
    install.insert_list.add(tab.assTypeId = ass_type_id);
  idP = db->insert(install);
  if (idP == 0) {
    DOODLE_LOG_WARN << fileStateP.c_str();
    throw std::runtime_error("");
  }
  coreDataManager::get().setAssInfoL(shared_from_this());
}

void assFileSqlInfo::updateSQL() {
  if (idP < 0) return;
  doodle::Basefile tab{};

  auto db = coreSql::getCoreSql().getConnection();
  auto updata = sqlpp::update(tab);
  updata.set(
      tab.infor = strList_tojson(infoP),
      tab.filestate = fileStateP
  ).where(tab.id == idP);
  db->update(updata);
}

assInfoPtrList assFileSqlInfo::getAll(const assClassPtr &AT_) {
  doodle::Basefile tab{};
  assInfoPtrList list;

  auto db = coreSql::getCoreSql().getConnection();
  for (auto &&row :db->run(sqlpp::select(sqlpp::all_of(tab))
                               .from(tab)
                               .where(tab.assClassId == AT_->getIdP())
                               .order_by(tab.filetime.desc())
  )) {
    auto assInfo = std::make_shared<assFileSqlInfo>();
    assInfo->batchSetAttr(row);
    assInfo->setAssClass(AT_);
    list.push_back(assInfo);
    assInfo->setAssType();
  }
  coreDataManager::get().setAssInfoL(list);
  return list;
}
dpath assFileSqlInfo::generatePath(const std::string &programFolder) {
//  QString path("%1/%2/%3/%4/%5");

  //第一次 格式化添加根目录
  dpath path = coreSet::getSet().getAssRoot();

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

  boost::format format("%1%%2%%3%");
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
  format % suffixes;

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
  return p_class_ptr_;
}

void assFileSqlInfo::setAssClass(const assClassPtr &class_ptr) {
  if (!class_ptr)
    return;
  p_class_ptr_ = class_ptr;
  ass_class_id = class_ptr->getIdP();

  setAssDep(class_ptr->getAssDep());
}

const assTypePtr &assFileSqlInfo::getAssType() {
  if (!p_type_ptr_) {
    for (const auto &item : coreDataManager::get().getAssTypeL()) {
      if (item->getIdP() == ass_type_id){
        p_type_ptr_ = item;
        break;
      }
    }
  }
  return p_type_ptr_;
}

void assFileSqlInfo::setAssType() {
  auto assTypeList = coreDataManager::get().getAssTypeL();
  for (const auto &item : assTypeList) {
    if (item->getIdP() == idP){
      p_type_ptr_ = item;
      break;
    }
  }
}
template<typename T>
void assFileSqlInfo::batchSetAttr(const T &row) {
  idP = row.id;
  fileP = row.file;
  fileSuffixesP = row.fileSuffixes;
  userP = row.user;
  versionP = row.version;
  filepathP = row.FilePath_;
  infoP = json_to_strList(row.infor);
  fileStateP = row.filestate;

  if (row.assClassId._is_valid)
    ass_class_id = row.assClassId;

  if (row.assTypeId._is_valid)
    ass_type_id = row.assTypeId;
}

CORE_NAMESPACE_E
