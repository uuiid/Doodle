#include <boost/format.hpp>
#include <memory>
#include <iostream>
#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>
#include "src/coreOrm/basefile_sqlOrm.h"
#include "src/shots/shottype.h"
#include "src/shots/shotClass.h"
#include "src/shots/shot.h"
#include "src/shots/episodes.h"
#include "src/core/coresql.h"
#include "src/shots/shotfilesqlinfo.h"
#include "filesqlinfo.h"

#include "src/core/coreset.h"

#include "Logger.h"

#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>

CORE_NAMESPACE_S

fileSqlInfo::fileSqlInfo()
    : coresqldata(),
      versionP(0),
      infoP(),
      fileSuffixesP(),
      fileP(),
      fileStateP(),
      filepathP(),
      userP(coreSet::getSet().getUser()),
      p_b_exist(true) {}

dpathList fileSqlInfo::getFileList() const {
  dpathList list_;
  try {
    nlohmann::json root = nlohmann::json::parse(filepathP);
    for (auto &&x : root) {
      list_.push_back(x.get<dstring>());
    }
  } catch (nlohmann::json::parse_error &error) {
    DOODLE_LOG_INFO << error.what();
    list_.push_back(filepathP);
  }
  return list_;
}

void fileSqlInfo::setFileList(const dpathList &filelist) {
  dstringList list;
  for (auto &&item : filelist) {
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

  for (auto &&x : filelist) {
    root.push_back(x);
  }
  filepathP = root.dump();
  fileP = boost::filesystem::basename(filelist[0]);
  fileSuffixesP = boost::filesystem::extension(filelist[0]);
}

int fileSqlInfo::getVersionP() const { return versionP; }

void fileSqlInfo::setVersionP(const int64_t &value) { versionP = value; }

dstringList fileSqlInfo::getInfoP() const { return infoP; }

void fileSqlInfo::setInfoP(const dstring &value) { infoP.push_back(value); }

dstring fileSqlInfo::getFileStateP() const { return fileStateP; }

void fileSqlInfo::setFileStateP(const dstring &value) { fileStateP = value; }

dstring fileSqlInfo::getUser() const { return userP; }

dstringList fileSqlInfo::json_to_strList(const dstring &json_str) const {
  dstringList list_;

  nlohmann::json root{};
  try {
    root = nlohmann::json::parse(json_str);
  } catch (nlohmann::json::parse_error &err) {
    DOODLE_LOG_INFO << err.what() << "无法解析";
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
  auto pro_path = coreSet::getSet().getPrjectRoot();
  if (refresh)
    for (const auto &path : getFileList()) {
      p_b_exist &= boost::filesystem::exists(pro_path / path);
    }
  return p_b_exist;
}

CORE_NAMESPACE_E
