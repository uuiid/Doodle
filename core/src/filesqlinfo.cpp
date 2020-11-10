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

#include <json/json.h>
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
  Json::Value root;
//  Json::StreamWriterBuilder builder;
//  builder.settings_["emitUTF8"] = true;
//  builder.settings_["commentStyle"] = "None";
//  builder.settings_["indentation"] = "";
  Json::CharReaderBuilder char_reader_builder;
  JSONCPP_STRING err;

  auto jsonReader = std::unique_ptr<Json::CharReader>(char_reader_builder.newCharReader());
  if (jsonReader->parse(fileStateP.c_str(), fileStateP.c_str() + fileStateP.length(),
                        &root, &err)) {
    for (auto &&x:root) {
      list_.push_back((dpath) x.asString());
    }
  } else {
    list_.push_back((dpath) fileStateP);
  }
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
  Json::StreamWriterBuilder builder;
  builder.settings_["emitUTF8"] = true;
  builder.settings_["commentStyle"] = "None";
  builder.settings_["indentation"] = "";

  Json::Value root;

  for (auto &&x:filelist) {
    root.append(x);
  }
  auto water = std::unique_ptr<Json::StreamWriter>(builder.newStreamWriter());
  filepathP = Json::writeString(builder, root);
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
  Json::Value root;
  Json::CharReaderBuilder char_reader_builder;
  JSONCPP_STRING err;

  auto jsonReader = std::unique_ptr<Json::CharReader>(char_reader_builder.newCharReader());
  if (jsonReader->parse(json_str.c_str(), json_str.c_str() + json_str.length(),
                        &root, &err)) {
    for (auto &&x:root) {
      list_.push_back(x.asString());
    }
  } else {
    list_.push_back(json_str);
  }
  return list_;

}
dstring fileSqlInfo::strList_tojson(const dstringList &str_list) const {
  Json::StreamWriterBuilder builder;
  builder.settings_["emitUTF8"] = true;
  builder.settings_["commentStyle"] = "None";
  builder.settings_["indentation"] = "";

  Json::Value root;

  for (auto &&x:str_list) {
    root.append(x);
  }
  auto water = std::unique_ptr<Json::StreamWriter>(builder.newStreamWriter());
  return Json::writeString(builder, root);
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
