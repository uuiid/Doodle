#include "coreset.h"
#include "coresql.h"

#include "src/convert.h"
#include "Logger.h"

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>
#include "coreOrm/configure_sqlOrm.h"
#include "coreOrm/project_sqlOrm.h"
#include "coreOrm/synfile_sqlOrm.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <json/json.h>

#include <stdexcept>
#include <fstream>

#include <QtCore/QStorageInfo>
CORE_NAMESPACE_S

const dstring coreSet::settingFileName = "doodle_conf.json";

coreSet &coreSet::getSet() {
  static coreSet install;
  return install;
}

void coreSet::init() {
  std::string str = std::getenv("HOMEPATH");
  *doc = "C:" + str + "/Documents/doodle";
  getSetting();
  initdb();
  getServerSetting();
  getCacheDiskPath();
}

void coreSet::initdb() {
  coreSql &sql = coreSql::getCoreSql();
  sql.initDB(ipMysql);
}

void coreSet::writeDoodleLocalSet() {
//  Json::CharReaderBuilder json;
  Json::Value root;
  Json::StreamWriterBuilder builder;
  builder.settings_["emitUTF8"] = true;
  const auto waiter = std::unique_ptr<Json::StreamWriter>(builder.newStreamWriter());

//  boost::filesystem::ifstream injosn((*doc/settingFileName),std::ifstream::binary);
//  injosn >> root;

  root["user"] = user;
  root["department"] = department;
  root["synPath"] = synPath->generic_string();
  root["synEp"] = syneps;
  root["projectname"] = project.second;
  root["FreeFileSync"] = freeFileSyn;

  boost::filesystem::ofstream outjosn;
  outjosn.open((*doc / settingFileName), std::ifstream::binary);
  waiter->write(root, &outjosn);
  outjosn.close();
//  boost::property_tree::ptree json;
//
//  boost::property_tree::read_json((*doc/settingFileName).string(),json);
//  json.put("", user);
//  json.put("department", department);
//  json.put("synPath", synPath);
//  json.put("synEp", syneps);
//  json.put("projectname",project.second);
//  json.put("FreeFileSync", freeFileSyn);
//
//  boost::property_tree::write_json((*doc/settingFileName).string(),json);
//
//  QJsonObject jsonobj;
//  jsonobj.insert("user", user);
//  jsonobj.insert("department", department);
//  jsonobj.insert("synPath", synPath.absolutePath());
//  jsonobj.insert("synEp", syneps);
//  jsonobj.insert("projectname", QString::fromStdString(project.second));
//  jsonobj.insert("FreeFileSync", freeFileSyn);
//  QJsonDocument jsonDoc(jsonobj);
//  QFile strFile(doc.absoluteFilePath(settingFileName));
//  if (!strFile.open(QIODevice::WriteOnly))
//    throw std::runtime_error("not open doodle_conf.json");
//  strFile.write(jsonDoc.toJson(QJsonDocument::Compact));
//  strFile.close();
}

coreSet::coreSet() {
  ipMysql = "192.168.10.213";
  user = "user";
  department = "VFX";
  syneps = 1;
  freeFileSyn = R"("C:\PROGRA~1\FREEFI~1\FreeFileSync.exe")";
  project = std::make_pair(1, "dubuxiaoyao3");
  synPath = std::make_shared<dpath>("D:/ue_prj");
  synServer = std::make_shared<dpath>("/03_Workflow/Assets");

  shotRoot = std::make_shared<dpath>("/03_Workflow/Shots");
  assRoot = std::make_shared<dpath>("/03_Workflow/Assets");
  prjectRoot = std::make_shared<dpath>("W:/");

  cacheRoot = std::make_shared<dpath>("C:/Doodle_cache");
  doc = std::make_shared<dpath>("C:/Doodle_cache");
}

void coreSet::getSetting() {
  if (boost::filesystem::exists(*doc / settingFileName)) {
    dpath strFile(*doc / settingFileName);
    boost::filesystem::ifstream inJosn;
    inJosn.open((*doc / settingFileName), std::ifstream::binary);

    Json::Value root;
    Json::CharReaderBuilder builder;
    builder.settings_["emitUTF8"] = true;
    JSONCPP_STRING errs;
    if (!Json::parseFromStream(builder, inJosn, &root, &errs)) {
      DOODLE_LOG_WARN << errs.c_str();
    }
    inJosn.close();
    if (root.isMember("user"))
      user = root["user"].asString();
    if (root.isMember("department"))
      department = root["department"].asString();
    if (root.isMember("synPath"))
      *synPath = root["synPath"].asString();
    if (root.isMember("synEp"))
      project.second = root["projectname"].asString();
    if (root.isMember("FreeFileSync"))
      freeFileSyn = root["FreeFileSync"].asString();
    DOODLE_LOG_INFO << root["projectname"].asString().c_str();
    DOODLE_LOG_INFO << root["synPath"].asString().c_str();
  }
}

dstring coreSet::toIpPath(const dstring &path) {
  static boost::regex exp("^[A-Z]:");
//  DOODLE_LOG_INFO << exp.match(path);

  if (boost::regex_match(path, exp)) {
    return path.substr(2);
  }
  return path;
}

dstring coreSet::getDepartment() const {
  return department;
}

void coreSet::setDepartment(const dstring &value) {
  department = value;
}

dstring coreSet::getUser() const {
  return user;
}

dstring coreSet::getUser_en() const {
  dopinyin::convert con;

  return boost::algorithm::to_lower_copy(con.toEn(user));//QString::fromStdString().toLower();
}

void coreSet::setUser(const dstring &value) {
  user = value;
}

dstring coreSet::getIpMysql() const {
  return ipMysql;
}

void coreSet::setIpMysql(const dstring &value) {
  ipMysql = value;
}

dstring coreSet::getIpFtp() const {
  return ipFTP;
}

void coreSet::setIpFtp(const dstring &value) {
  ipFTP = value;
}

dpath coreSet::getDoc() const {
  return *doc;
}

dpath coreSet::getCacheRoot() const {
  return *cacheRoot;
}

dpath coreSet::getPrjectRoot() const {
  return *prjectRoot;
}

void coreSet::setPrjectRoot(const dpath &value) {
  *prjectRoot = value;
}

dpath coreSet::getAssRoot() const {
  return *assRoot;
}

void coreSet::setAssRoot(const dpath &value) {
  *assRoot = value;
}

dpath coreSet::getShotRoot() const {
  return *shotRoot;
}

void coreSet::setShotRoot(const dpath &value) {
  shotRoot = std::make_shared<dpath>(value);
}

dstring coreSet::getProjectname() {
  return project.second;
}

dstring coreSet::getFreeFileSyn() const {
  return freeFileSyn;
}

void coreSet::setFreeFileSyn(const dstring &value) {
  freeFileSyn = value;
}

int coreSet::getSyneps() const {
  return syneps;
}

void coreSet::setSyneps(int value) {
  syneps = value;
}

void coreSet::getServerSetting() {
  //获得项目个数
  auto db = coreSql::getCoreSql().getConnection();
  doodle::Project prjTab{};
  for (auto &&raw: db->run(sqlpp::select(sqlpp::all_of(prjTab)).from(prjTab).unconditionally())) {
    prjMap.insert(std::make_pair<int, std::string>((int) raw.id, (std::string) raw.name));
  }
  setProjectname(project.second);

  doodle::Configure tab{};
  std::map<std::string, std::string> map;
  for (auto &&raw: db->run(sqlpp::select(tab.name, tab.value)
                               .from(tab)
                               .where(tab.projectId == project.first))) {
    map.insert(std::make_pair<std::string, std::string>(raw.name, raw.value));
    DOODLE_LOG_INFO << raw.name.text << "--->" << raw.value.text;
  }

  *shotRoot = (map["shotRoot"]);
  *assRoot = (map["assetsRoot"]);
  *synServer = (map["synSever"]);
  *prjectRoot = (map["project"]);

  if (map.find("IP_FTP") != map.end())
    ipFTP = (map["IP_FTP"]);
  else
    ipFTP = ipMysql;
}

synPathListPtr coreSet::getSynDir() {
  auto db = coreSql::getCoreSql().getConnection();
  doodle::Synfile table{};
  Json::CharReaderBuilder builder;
  Json::Value root;
  JSONCPP_STRING err;
  synPathListPtr list;
  const auto reand = std::unique_ptr<Json::CharReader>(builder.newCharReader());
  for (auto &&row:db->run(sqlpp::select(table.path)
                              .from(table)
                              .where(table.episodesId == syneps))) {
    reand->parse(row.path.text,row.path.text + row.path.len,&root,&err);
    for (const auto &item : root) {
      synPath_struct fileSyn{};
      fileSyn.local = std::make_shared<dpath>(item["Left"].asString());
      fileSyn.server = std::make_shared<dpath>(item["Right"].asString());
      list.push_back(fileSyn);
    }
    DOODLE_LOG_INFO << QString::fromStdString(row.path);
  }
  return list;
}

void coreSet::getCacheDiskPath() {
  for (QStorageInfo &x : QStorageInfo::mountedVolumes()) {
    if (x.isValid() && x.isReady()) {
      if (!x.isReadOnly()) {
        if (((double) x.bytesAvailable() / (double) x.bytesTotal() > 0.5f) && (!x.isRoot())) {
          *cacheRoot = x.rootPath().toStdString() + "Doodle_cache";
          break;
        }
      }
    }
  }
}
dstringList coreSet::getAllPrjName() const {
  dstringList list;
  for (auto &&prj :prjMap) {
    list.push_back(prj.second);
  }
  return list;
}
const dpath &coreSet::getSynPathLocale() const {
  return *synPath;
}
void coreSet::setSynPathLocale(const dpath &syn_path) {
  synPath = std::make_shared<dpath>(syn_path);
}
std::pair<int, std::string> coreSet::projectName() const {
  return project;
}
void coreSet::setProjectname(const std::string &value) {
  auto result = std::find_if(prjMap.begin(), prjMap.end(),
                             [=](const auto &mo) { return mo.second == value; });
  if (result != prjMap.end())
    project = *result;
}
CORE_NAMESPACE_E
