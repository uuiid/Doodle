#include "filesqlinfo.h"
#include <corelib/fileDBInfo/pathParsing.h>

#include <corelib/core/coreset.h>
#include <corelib/shots/shottype.h>
#include <corelib/shots/shotClass.h>
#include <corelib/shots/shot.h>
#include <corelib/shots/episodes.h>
#include <corelib/core/coresql.h>
#include <corelib/shots/shotfilesqlinfo.h>
#include <corelib/fileDBInfo/CommentInfo.h>

#include <loggerlib/Logger.h>

#include <corelib/filesystem/FileSystem.h>

//orm库
#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>
#include <corelib/coreOrm/basefile_sqlOrm.h>

#include <boost/format.hpp>
#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>

//反射使用
#include <rttr/registration>

#include <memory>
#include <iostream>
DOODLE_NAMESPACE_S

RTTR_REGISTRATION {
  rttr::registration::class_<fileSqlInfo>(DOCORE_RTTE_CLASS(fileSqlInfo))
      // .property("filelist", &fileSqlInfo::getFileList,
      //           rttr::select_overload<void(const dpathList &)>(&fileSqlInfo::setFileList))
      .property("version", &fileSqlInfo::getVersionP, &fileSqlInfo::setVersionP)
      // .property("info", &fileSqlInfo::getInfoP, &fileSqlInfo::setInfoP)
      .property_readonly("user", &fileSqlInfo::getUser)
      .property_readonly("suffixes", &fileSqlInfo::getSuffixes)
      .method("generatePath",
              rttr::select_overload<dpath(const std::string &)>(&fileSqlInfo::generatePath))
      .method("generatePath",
              rttr::select_overload<dpath(const std::string &,
                                          const std::string &)>(&fileSqlInfo::generatePath))
      .method("generatePath",
              rttr::select_overload<dpath(const std::string &,
                                          const std::string &,
                                          const std::string &)>(&fileSqlInfo::generatePath))
      .method("generateFileName",
              rttr::select_overload<dstring(const std::string &)>(&fileSqlInfo::generateFileName))
      .method("generateFileName",
              rttr::select_overload<dstring(const std::string &,
                                            const std::string &)>(&fileSqlInfo::generateFileName))
      .method("deleteSQL", &fileSqlInfo::deleteSQL)
      .method("exist", &fileSqlInfo::exist);
}

fileSqlInfo::fileSqlInfo()
    : coresqldata(),
      versionP(0),
      fileSuffixesP(),
      fileP(),
      fileStateP(),
      userP(coreSet::getSet().getUser()),
      p_b_exist(false),
      //私有属性
      p_parser_path(),
      p_parser_info() {
  p_parser_info = std::make_shared<CommentInfo>(this);
  p_parser_path = std::make_shared<pathParsing>(this);
}

dpathList fileSqlInfo::getFileList() const {
  return p_parser_path->Path();
}

dpathList fileSqlInfo::getFileList() {
  return p_parser_path->Path();
}

void fileSqlInfo::setFileList(const dpathList &filelist) {
  if (filelist.empty()) {
    DOODLE_LOG_WARN("传入空列表");
    return;
  }
  p_parser_path->setPath(filelist);
  fileP         = boost::filesystem::basename(filelist[0]);
  fileSuffixesP = boost::filesystem::extension(filelist[0]);
}

void fileSqlInfo::setFileList(const dstringList &filelist) {
  dstringList list;
  for (auto &&item : filelist) {
    list.push_back(item);
  }
  setFileList(list);
}

int64_t fileSqlInfo::getVersionP() const { return versionP; }

void fileSqlInfo::setVersionP(const int64_t value) { versionP = value; }

dstringList fileSqlInfo::getInfoP() const { return p_parser_info->Info(); }

void fileSqlInfo::setInfoP(const dstring &value) { p_parser_info->setInfo(value); }

dstring fileSqlInfo::getFileStateP() const { return fileStateP; }

void fileSqlInfo::setFileStateP(const dstring &value) { fileStateP = value; }

dstring fileSqlInfo::getUser() const { return userP; }

void fileSqlInfo::insert() {
  exist(true);
}

void fileSqlInfo::updateSQL() {
  exist(true);
}

dstring fileSqlInfo::getSuffixes() const { return fileSuffixesP; }

void fileSqlInfo::deleteSQL() {
  doodle::Basefile tab{};

  auto db = coreSql::getCoreSql().getConnection();
  db->remove(sqlpp::remove_from(tab).where(tab.id == idP));
}

bool fileSqlInfo::exist(bool refresh) {
  if (refresh) {
    p_b_exist = true;
    for (const auto &path : getFileList()) {
      p_b_exist &= DfileSyntem::get().exists(path);
    }
  }
  return p_b_exist;
}

void fileSqlInfo::write() {
  p_parser_path->write();
  p_parser_info->write();
}

DOODLE_NAMESPACE_E
