#include "filesqlinfo.h"
#include <src/fileDBInfo/pathParsing.h>

#include <src/core/coreset.h>
#include <src/shots/shottype.h>
#include <src/shots/shotClass.h>
#include <src/shots/shot.h>
#include <src/shots/episodes.h>
#include <src/core/coresql.h>
#include <src/shots/shotfilesqlinfo.h>

#include <Logger.h>

#include <src/DfileSyntem.h>

//orm库
#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>
#include <src/coreOrm/basefile_sqlOrm.h>

#include <boost/format.hpp>
#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>

//反射使用
#include <rttr/registration>

#include <memory>
#include <iostream>
CORE_NAMESPACE_S

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
      infoP(),
      fileSuffixesP(),
      fileP(),
      fileStateP(),
      filepathP(),
      userP(coreSet::getSet().getUser()),
      p_b_exist(false),
      //私有属性
      p_parser(),
      p_pathlist() {}

dpathList fileSqlInfo::getFileList() const {
  return p_pathlist;

  // dpathList list_;
  // try {
  //   nlohmann::json root = nlohmann::json::parse(filepathP);
  //   for (auto &&x : root) {
  //     list_.push_back(x.get<dstring>());
  //   }
  // } catch (nlohmann::json::parse_error &error) {
  //   DOODLE_LOG_INFO(error.what());
  //   list_.push_back(filepathP);
  // }
  // return list_;
}

dpathList fileSqlInfo::getFileList() {
  if (p_pathlist.empty()) {
    parsepath(filepathP);
  }
  return p_pathlist;
}

void fileSqlInfo::setFileList(const dpathList &filelist) {
  if (filelist.empty()) {
    DOODLE_LOG_WARN("传入空列表");
    return;
  }

  p_pathlist.clear();
  for (auto &&path : filelist) {
    p_pathlist.push_back(path);
  }

  dpath path{generatePath("doodle", ".json")};

  if (!p_parser) {
    //成功获得了路径
    if (!path.empty()) {
      p_parser = std::make_shared<pathParsing>();
    }
  }

  // nlohmann::json root;

  // for (auto &&x : filelist) {
  //   root.push_back(x);
  // }
  filepathP     = path.generic_string();
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

dstringList fileSqlInfo::getInfoP() const { return infoP; }

void fileSqlInfo::setInfoP(const dstring &value) { infoP.push_back(value); }

dstring fileSqlInfo::getFileStateP() const { return fileStateP; }

void fileSqlInfo::setFileStateP(const dstring &value) { fileStateP = value; }

dstring fileSqlInfo::getUser() const { return userP; }

void fileSqlInfo::insert() {
  if (p_parser) {
    p_parser->write(p_pathlist, generatePath("doodle", ".json"));
  }
  exist(true);
}

void fileSqlInfo::updateSQL() {
  if (p_parser) {
    p_parser->write(p_pathlist, generatePath("doodle", ".json"));
  }
  exist(true);
}

dstringList fileSqlInfo::json_to_strList(const dstring &json_str) const {
  dstringList list_;

  if (json_str.empty()) {
    list_.push_back("");
    return list_;
  }

  nlohmann::json root{};
  try {
    root = nlohmann::json::parse(json_str);
  } catch (nlohmann::json::parse_error &err) {
    DOODLE_LOG_INFO(err.what() << "not parse json : " << json_str);
  }
  if (!root.empty()) {
    for (auto &&x : root) {
      list_.push_back(x.get<dstring>());
    }
  } else {
    list_.push_back(json_str);
  }
  if (list_.empty()) {
    list_.push_back("");
  }
  return list_;
}
dstring fileSqlInfo::strList_tojson(const dstringList &str_list) const {
  nlohmann::json root;

  for (auto &&x : str_list) {
    root.push_back(x);
  }
  return root.dump();
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
      p_b_exist &= doSystem::DfileSyntem::get().exists(path);
    }
  }
  return p_b_exist;
}

void fileSqlInfo::parsepath(const std::string &pathstr) {
  if (!p_parser) {
    p_parser = std::make_shared<pathParsing>();
  }
  p_pathlist = p_parser->getPath(pathstr);
}

CORE_NAMESPACE_E
