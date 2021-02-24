#include "assfilesqlinfo.h"
#include <corelib/Exception/Exception.h>

#include <corelib/core/coreset.h>
#include <corelib/core/coresql.h>

#include <corelib/assets/assdepartment.h>
#include <corelib/assets/assClass.h>
#include <corelib/assets/assType.h>

#include <corelib/fileDBInfo/CommentInfo.h>
#include <corelib/fileDBInfo/pathParsing.h>
#include <loggerlib/Logger.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/sqlite3.h>

#include <iostream>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>

//反射使用
#include <rttr/registration>

DOODLE_NAMESPACE_S

RTTR_REGISTRATION {
  rttr::registration::class_<assFileSqlInfo>(DOCORE_RTTE_CLASS(assFileSqlInfo))
      .constructor<>()(rttr::policy::ctor::as_std_shared_ptr);
}

DOODLE_INSRANCE_CPP(assFileSqlInfo);
boost::signals2::signal<void(const assInfoPtr &)> assFileSqlInfo::insertChanged{};
boost::signals2::signal<void(const assInfoPtr &)> assFileSqlInfo::updateChanged{};

assFileSqlInfo::assFileSqlInfo()
    : fileSqlInfo(),
      std::enable_shared_from_this<assFileSqlInfo>(),
      p_type_ptr_(),
      p_class_ptr_(),
      p_dep_ptr_() {
  p_instance.insert(this);
}

assFileSqlInfo::~assFileSqlInfo() {
  p_instance.erase(this);
}

bool assFileSqlInfo::setInfo(const std::string &value) {
  return true;
}

assInfoPtrList assFileSqlInfo::getAll(const assClassPtr &AT_) {
  return {};
}
fileSys::path assFileSqlInfo::generatePath(const std::string &programFolder) {
  //  QString path("%1/%2/%3/%4/%5");

  //第一次 格式化添加根目录
  fileSys::path path = "";  //coreSet::getSet().getAssRoot();

  //第二次添加类型
  auto dep = getAssDep();
  if (dep)
    path = path / dep->getAssDep();
  else
    throw nullptr_error("assFileSqlInfo err");

  //第三次格式化添加  ass_type
  auto as_cls = getAssClass();
  if (as_cls)
    path = path / as_cls->getAssClass();
  else
    throw nullptr_error("assFileSqlInfo err");

  //第四次次格式化程序文件夹
  path = path / programFolder;

  //第五次添加fileType
  auto as_ty = getAssType();
  if (as_ty) {
    path = path / as_ty->getTypeS();
  } else
    throw nullptr_error("assFileSqlInfo err");

  return path;
}

fileSys::path assFileSqlInfo::generatePath(const dstring &programFolder,
                                           const dstring &suffixes) {
  return generatePath(programFolder) / generateFileName(suffixes);
}

fileSys::path assFileSqlInfo::generatePath(const dstring &programFolder,
                                           const dstring &suffixes,
                                           const dstring &prefix) {
  return generatePath(programFolder) / generateFileName(suffixes, prefix);
}

dstring assFileSqlInfo::generateFileName(const dstring &suffixes) {
  boost::format format("%1%_%2%%3%");
  auto as_cl = getAssClass();
  if (as_cl)
    format % as_cl->getAssClass();
  else
    throw nullptr_error("assFileSqlInfo err");

  auto as_ty = getAssType();
  if (as_ty) {
    if (as_ty->getTypeS() == "rig")
      format % as_ty->getTypeS();
    else
      format % "";
  } else
    throw nullptr_error("assFileSqlInfo err");

  format % suffixes;

  return format.str();
}

dstring assFileSqlInfo::generateFileName(const dstring &suffixes,
                                         const dstring &prefix) {
  boost::format str("%1%_%2%");
  str % prefix % generateFileName(suffixes);
  return str.str();
}

assDepPtr assFileSqlInfo::getAssDep() { return p_dep_ptr_; }

void assFileSqlInfo::setAssDep(const assDepPtr &ass_dep_) {
  if (!ass_dep_) return;
  p_dep_ptr_ = ass_dep_;
}

const assClassPtr &assFileSqlInfo::getAssClass() { return p_class_ptr_; }

void assFileSqlInfo::setAssClass(const assClassPtr &class_ptr) {
  if (!class_ptr) return;
  p_class_ptr_ = class_ptr;

  setAssDep(class_ptr->getAssDep());
}

const assTypePtr &assFileSqlInfo::getAssType() {
  return p_type_ptr_;
}

void assFileSqlInfo::setAssType() {
}
void assFileSqlInfo::setAssType(const assTypePtr &type_ptr) {
  p_type_ptr_ = type_ptr;

  versionP = getMaxVecsion() + 1;
}

dataInfoPtr assFileSqlInfo::findSimilar() {
  auto it =
      std::find_if(
          p_instance.begin(), p_instance.end(),
          [=](const assFileSqlInfo *part) -> bool {
            return part->p_dep_ptr_ == p_dep_ptr_;
          });
  if (it != p_instance.end()) {
    (*it)->fileP         = fileP;
    (*it)->fileStateP    = fileStateP;
    (*it)->fileSuffixesP = fileSuffixesP;
    (*it)->versionP      = versionP;
    (*it)->userP         = userP;
    (*it)->p_parser_info = p_parser_info;
    (*it)->p_parser_path = p_parser_path;

    p_parser_info->setFileSql(*it);
    p_parser_path->setFileSql(*it);
    return (*it)->shared_from_this();
  } else
    return shared_from_this();
}

bool assFileSqlInfo::sortType(const assInfoPtr &t1, const assInfoPtr &t2) {
  auto t1_type = t1->getAssType();
  auto t2_type = t2->getAssType();
  if (t1_type && t2_type) {
    return t1_type->getTypeS() < t2_type->getTypeS();
  } else {
    return false;
  }
}
int assFileSqlInfo::getMaxVecsion() {
  for (const auto &info_l : p_instance) {
    if (getAssType() == info_l->getAssType())
      return info_l->versionP;
  }
  return 0;
}
const std::unordered_set<assFileSqlInfo *> assFileSqlInfo::Instances() {
  return p_instance;
}
DOODLE_NAMESPACE_E
