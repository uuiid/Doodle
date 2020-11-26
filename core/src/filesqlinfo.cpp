#include <boost/format.hpp>
#include <memory>
#include <iostream>
#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>
#include "coreOrm/basefile_sqlOrm.h"
#include "shottype.h"
#include "shotClass.h"
#include "shot.h"
#include "episodes.h"
#include "coresql.h"
#include "shotfilesqlinfo.h"
#include "filesqlinfo.h"

#include "coreset.h"

#include "Logger.h"

#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>


CORE_NAMESPACE_S

fileSqlInfo::fileSqlInfo() :
    coresqldata(),
    versionP(0),
    infoP(),
    fileSuffixesP(),
    fileP(),
    fileStateP(),
    filepathP(),
    userP(coreSet::getSet().getUser()){
}

dpathList fileSqlInfo::getFileList() const {
  dpathList list_;
//  nlohmann::json root;
//  auto jsonReader = std::unique_ptr<Json::CharReader>(char_reader_builder.newCharReader());
//  const size_t len = filepathP.length();
//  root = filepathP;
  try {
    nlohmann::json root = nlohmann::json::parse(filepathP);
    for (auto &&x:root) {
      list_.push_back(x.get<dstring>());
    }
  } catch (nlohmann::json::parse_error & error) {
    DOODLE_LOG_INFO << error.what();
    list_.push_back(filepathP);
  }
//  if (!root.empty()) {
//    for (auto &&x:root) {
//      list_.push_back(x.get<dstring>());
//    }
//  } else {
//    list_.push_back(filepathP);
//  }
  return list_;
}

void fileSqlInfo::setFileList(const dpathList &filelist) {
  dstringList list;
  for (auto &&item:filelist) {
    list.push_back(item.generic_string());
  }
  setFileList(list);
}

void fileSqlInfo::setFileList(const dstringList &filelist) {
  if (filelist.empty()) {
    DOODLE_LOG_WARN << "传入空列表";
    return;
  }

  nlohmann::json root;

  for (auto &&x:filelist) {
    root.push_back(x);
  }
  filepathP = root.dump();
  fileP = boost::filesystem::basename(filelist[0]);
  fileSuffixesP = boost::filesystem::extension(filelist[0]);
}

int fileSqlInfo::getVersionP() const {
  return versionP;
}

void fileSqlInfo::setVersionP(const int64_t &value) {
  versionP = value;
}

dstringList fileSqlInfo::getInfoP() const {
  return infoP;
}

void fileSqlInfo::setInfoP(const dstring &value) {
  infoP.push_back(value);
}

dstring fileSqlInfo::getFileStateP() const {
  return fileStateP;
}

void fileSqlInfo::setFileStateP(const dstring &value) {
  fileStateP = value;
}
dstring fileSqlInfo::getUser() const {
  return userP;
}
dstringList fileSqlInfo::json_to_strList(const dstring &json_str) const {
  dstringList list_;

  nlohmann::json root  = json_str;
  if (!root.empty()) {
    for (auto &&x:root) {
      list_.push_back(x.get<dstring>());
    }
  } else {
    list_.push_back(json_str);
  }
  if (list_.empty())
  {
    list_.push_back("");
  }
  return list_;

}
dstring fileSqlInfo::strList_tojson(const dstringList &str_list) const {

  nlohmann::json root;

  for (auto &&x:str_list) {
    root.push_back(x);
  }
  return root.dump();
}
dstring fileSqlInfo::getSuffixes() const {
  return fileSuffixesP;
}
void fileSqlInfo::deleteSQL() {
  doodle::Basefile tab{};

  auto db = coreSql::getCoreSql().getConnection();
  db->remove(sqlpp::remove_from(tab)
                 .where(tab.id == idP));
}

CORE_NAMESPACE_E
