#include "coreset.h"
#include <corelib/core/coresql.h>

#include <pinyinlib/convert.h>
#include <loggerlib/Logger.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>
#include <corelib/coreOrm/configure_sqlOrm.h>
#include <corelib/coreOrm/project_sqlOrm.h>
#include <corelib/coreOrm/synfile_sqlOrm.h>
#include <corelib/coreOrm/user_sqlOrm.h>
#include <corelib/coreOrm/episodes_sqlOrm.h>

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <fstream>

#include <QtCore/QStorageInfo>

#include <corelib/filesystem/FileSystem.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/process.hpp>
#include <boost/dll.hpp>
#include <boost/dll/runtime_symbol_info.hpp>

#include <magic_enum.hpp>

DOODLE_NAMESPACE_S

const dstring coreSet::settingFileName = "doodle_conf.json";

coreSet &coreSet::getSet() {
  static coreSet install;
  return install;
}

void coreSet::init() {
  //这里我们手动做一些工作
  char *k_path;
  size_t k_path_len;
  getenv_s(&k_path_len, NULL, 0, "HOMEPATH");
  if (k_path_len == 0)
    *doc = "C:/doodle/Documents/doodle";
  k_path = (char *)malloc(k_path_len * sizeof(char));
  if (!k_path)
    *doc = "C:/doodle/Documents/doodle";
  getenv_s(&k_path_len, k_path, k_path_len, "HOMEPATH");

  std::string str(k_path);  //std::getenv("HOMEPATH")
  free(k_path);

  *doc = "C:" + str + "/Documents/doodle";
  getSetting();
  initdb();
  getServerSetting();
  getCacheDiskPath();
  appendEnvironment();
  DOODLE_LOG_INFO("登录 : " << project.second.c_str());
  DfileSyntem::get().session(ipFTP, 6666, project.second, "", project.second);
  if (!boost::filesystem::exists(getCacheRoot())) {
    boost::filesystem::create_directories(getCacheRoot());
  }
}

void coreSet::reInit() {
  initdb();
  getServerSetting();
  DfileSyntem::get().session(ipFTP, 21, project.second, "", project.second);
}

void coreSet::initdb() {
  coreSql &sql = coreSql::getCoreSql();
  sql.initDB(ipMysql);
}

void coreSet::appendEnvironment() const {
  auto env          = boost::this_process::environment();
  auto this_process = program_location();
  env["PATH"] += (this_process.parent_path() / "tools/ffmpeg/bin").generic_string();
  if (boost::filesystem::exists(R"(C:\Program Files\Autodesk\Maya2018\bin)")) {
    env["PATH"] += R"(C:\Program Files\Autodesk\Maya2018\bin\)";
  } else if (boost::filesystem::exists(R"(C:\Program Files\Autodesk\Maya2019\bin)")) {
    env["PATH"] += R"(C:\Program Files\Autodesk\Maya2019\bin\)";
  } else if (boost::filesystem::exists(R"(C:\Program Files\Autodesk\Maya2020\bin)")) {
    env["PATH"] += R"(C:\Program Files\Autodesk\Maya2020\bin\)";
  }
}

void coreSet::writeDoodleLocalSet() {
  nlohmann::json root;

  root["user"]         = user;
  root["department"]   = getDepartment();
  root["synPath"]      = synPath->generic_string();
  root["synEp"]        = syneps;
  root["projectname"]  = project.second;
  root["FreeFileSync"] = freeFileSyn;

  boost::filesystem::ofstream outjosn;
  outjosn.open((*doc / settingFileName), std::ifstream::binary);
  outjosn << root.dump();
  outjosn.close();
}

coreSet::coreSet()
    : ipMysql("192.168.10.213"),
      ipFTP("192.168.10.213"),
      user("user"),
      department(Department::VFX),
      syneps(1),
      freeFileSyn(R"("C:\PROGRA~1\FREEFI~1\FreeFileSync.exe")"),
      project(std::make_pair(1, "dubuxiaoyao3")),
      synPath(std::make_shared<dpath>("D:/ue_prj")),
      prjMap(),
      shotRoot(std::make_shared<dpath>("/03_Workflow/Shots")),
      assRoot(std::make_shared<dpath>("/03_Workflow/Assets")),
      prjectRoot(std::make_shared<dpath>("W:/")),
      cacheRoot(std::make_shared<dpath>("C:/Doodle_cache")),
      doc(std::make_shared<dpath>("C:/Doodle_cache")) {
}

void coreSet::getSetting() {
  if (boost::filesystem::exists(*doc / settingFileName)) {
    dpath strFile(*doc / settingFileName);
    boost::filesystem::ifstream inJosn;
    inJosn.open((*doc / settingFileName), std::ifstream::binary);

    nlohmann::json root = nlohmann::json::parse(inJosn);
    inJosn.close();

    user           = root.value("user", "uset_tmp");
    auto k_dep     = root.value("department", "VFX");
    department     = magic_enum::enum_cast<Department>(k_dep).value_or(Department::VFX);
    *synPath       = root.value("synPath", "D:/ue_prj");
    project.second = root.value("synEp", 1);
    freeFileSyn    = root.value("FreeFileSync",
                             R"("C:\PROGRA~1\FREEFI~1\FreeFileSync.exe")");
    project.second = root.value("projectname", "dubuxiaoyao3");
    syneps         = root.value("synEp", 1);

    DOODLE_LOG_INFO("projectname" << project.second.c_str());
  }
}

dstring coreSet::toIpPath(const dstring &path) {
  static boost::regex exp("^[A-Z]:");

  if (boost::regex_search(path, exp)) {
    return path.substr(2);
  }
  return path;
}

dstring coreSet::getDepartment() const {
  return std::string{magic_enum::enum_name(department)};
}

void coreSet::setDepartment(const dstring &value) {
  department = magic_enum::enum_cast<Department>(value).value_or(Department::VFX);
}

dstring coreSet::getUser() const { return user; }

dstring coreSet::getUser_en() const {
  return boost::algorithm::to_lower_copy(
      dopinyin::convert::Get().toEn(user));
}

void coreSet::setUser(const dstring &value) { user = value; }

dstring coreSet::getIpMysql() const { return ipMysql; }

void coreSet::setIpMysql(const dstring &value) { ipMysql = value; }

dstring coreSet::getIpFtp() const { return ipFTP; }

void coreSet::setIpFtp(const dstring &value) { ipFTP = value; }

dpath coreSet::getDoc() const { return *doc; }

dpath coreSet::getCacheRoot() const { return *cacheRoot; }

dpath coreSet::getPrjectRoot() const { return *prjectRoot; }

void coreSet::setPrjectRoot(const dpath &value) { *prjectRoot = value; }

dpath coreSet::getAssRoot() const { return *assRoot; }

void coreSet::setAssRoot(const dpath &value) { *assRoot = value; }

dpath coreSet::getShotRoot() const { return *shotRoot; }

void coreSet::setShotRoot(const dpath &value) {
  shotRoot = std::make_shared<dpath>(value);
}

dstring coreSet::getProjectname() { return project.second; }

dstring coreSet::getFreeFileSyn() const { return freeFileSyn; }

void coreSet::setFreeFileSyn(const dstring &value) { freeFileSyn = value; }

int coreSet::getSyneps() const { return syneps; }

void coreSet::setSyneps(int value) { syneps = value; }

void coreSet::getServerSetting() {
  //获得项目个数
  auto db = coreSql::getCoreSql().getConnection();
  doodle::Project prjTab{};
  for (auto &&raw : db->run(sqlpp::select(sqlpp::all_of(prjTab))
                                .from(prjTab)
                                .unconditionally())) {
    prjMap.insert(
        std::make_pair<int, std::string>((int)raw.id, (std::string)raw.name));
  }
  setProjectname(project.second);

  doodle::Configure tab{};
  std::map<std::string, std::string> map;
  for (auto &&raw : db->run(sqlpp::select(tab.name, tab.value)
                                .from(tab)
                                .where(tab.projectId == project.first))) {
    map.insert(std::make_pair<std::string, std::string>(raw.name, raw.value));
    DOODLE_LOG_INFO(raw.name.text << "--->" << raw.value.text << "\n");
  }

  *shotRoot   = map["shotRoot"];
  *assRoot    = map["assetsRoot"];
  *prjectRoot = map["project"];

  if (map.find("IP_FTP") != map.end())
    ipFTP = (map["IP_FTP"]);
  else
    ipFTP = ipMysql;
}

void coreSet::getCacheDiskPath() {
  for (QStorageInfo &x : QStorageInfo::mountedVolumes()) {
    if (x.isValid() && x.isReady()) {
      if (!x.isReadOnly()) {
        if (((double)x.bytesAvailable() / (double)x.bytesTotal() > 0.02f) &&
            (!x.isRoot())) {
          *cacheRoot = x.rootPath().toStdString() + "Doodle_cache";
          break;
        }
      }
    }
  }
}

dstringList coreSet::getAllPrjName() const {
  dstringList list;
  for (auto &&prj : prjMap) {
    list.push_back(prj.second);
  }
  return list;
}
const dpath coreSet::getSynPathLocale() const { return *synPath; }
void coreSet::setSynPathLocale(const dpath &syn_path) {
  synPath = std::make_shared<dpath>(syn_path);
}

std::pair<int, std::string> coreSet::projectName() const { return project; }

void coreSet::setProjectname(const std::string &value) {
  auto result = std::find_if(prjMap.begin(), prjMap.end(), [=](const auto &mo) {
    return mo.second == value;
  });
  if (result != prjMap.end()) project = *result;

  DOODLE_LOG_INFO(project.first << project.second);
}
dpath coreSet::program_location() {
  return boost::dll::program_location().parent_path();
}
dpath coreSet::program_location(const dpath &path) {
  return program_location() / path;
}
dstringList coreSet::getAllUser() {
  auto db = coreSql::getCoreSql().getConnection();
  doodle::User table{};
  dstringList dstring_list{};
  for (const auto &row :
       db->run(sqlpp::select(table.user).from(table).unconditionally())) {
    dstring_list.push_back(row.user);
  }

  return dstring_list;
}
bool coreSet::subUser(const dstring &user_str) {
  if (user_str.empty()) return false;
  auto db = coreSql::getCoreSql().getConnection();
  doodle::User table{};

  auto pow =
      boost::algorithm::to_lower_copy(dopinyin::convert::Get().toEn(user_str));
  db->run(sqlpp::insert_into(table).set(table.user     = user_str,
                                        table.password = pow));
  return true;
}
DOODLE_NAMESPACE_E
