#include "coreset.h"
#include <src/core/coresql.h>

#include <src/convert.h>
#include <Logger.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>
#include <src/coreOrm/configure_sqlOrm.h>
#include <src/coreOrm/project_sqlOrm.h>
#include <src/coreOrm/synfile_sqlOrm.h>
#include <src/coreOrm/user_sqlOrm.h>
#include <src/coreOrm/episodes_sqlOrm.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <fstream>

#include <QtCore/QStorageInfo>
#include <boost/dll.hpp>
#include <fileSystem_cpp.h>

#include <boost/process.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
CORE_NAMESPACE_S

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
  doSystem::DfileSyntem::getFTP().session(ipFTP, 21, project.second, "", *prjectRoot);
  if (!boost::filesystem::exists(getCacheRoot())) {
    boost::filesystem::create_directories(getCacheRoot());
  }
}

void coreSet::reInit() {
  initdb();
  getServerSetting();
  doSystem::DfileSyntem::getFTP().session(ipFTP, 21, project.second, "", *prjectRoot);
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
  root["department"]   = department;
  root["synPath"]      = synPath->generic_string();
  root["synEp"]        = syneps;
  root["projectname"]  = project.second;
  root["FreeFileSync"] = freeFileSyn;

  boost::filesystem::ofstream outjosn;
  outjosn.open((*doc / settingFileName), std::ifstream::binary);
  outjosn << root.dump();
  outjosn.close();
}

coreSet::coreSet() {
  ipMysql     = "192.168.10.213";
  user        = "user";
  department  = "VFX";
  syneps      = 1;
  freeFileSyn = R"("C:\PROGRA~1\FREEFI~1\FreeFileSync.exe")";
  project     = std::make_pair(1, "dubuxiaoyao3");
  synPath     = std::make_shared<dpath>("D:/ue_prj");
  synServer   = std::make_shared<dpath>("/03_Workflow/Assets");

  shotRoot   = std::make_shared<dpath>("/03_Workflow/Shots");
  assRoot    = std::make_shared<dpath>("/03_Workflow/Assets");
  prjectRoot = std::make_shared<dpath>("W:/");

  cacheRoot = std::make_shared<dpath>("C:/Doodle_cache");
  doc       = std::make_shared<dpath>("C:/Doodle_cache");
}

void coreSet::getSetting() {
  if (boost::filesystem::exists(*doc / settingFileName)) {
    dpath strFile(*doc / settingFileName);
    boost::filesystem::ifstream inJosn;
    inJosn.open((*doc / settingFileName), std::ifstream::binary);

    nlohmann::json root;
    std::stringstream instr;
    instr << inJosn.rdbuf();
    instr >> root;

    inJosn.close();

    user = root.value("user", "");

    department = root.value("department", "VFX");

    *synPath = root.value("synPath", "D:/ue_prj");

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

  if (boost::regex_match(path, exp)) {
    return path.substr(2);
  }
  return path;
}

dstring coreSet::getDepartment() const { return department; }

void coreSet::setDepartment(const dstring &value) { department = value; }

dstring coreSet::getUser() const { return user; }

dstring coreSet::getUser_en() const {
  dopinyin::convert con;
  return boost::algorithm::to_lower_copy(
      con.toEn(user));  // QString::fromStdString().toLower();
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

  *shotRoot   = (map["shotRoot"]);
  *assRoot    = (map["assetsRoot"]);
  *synServer  = (map["synSever"]);
  *prjectRoot = (map["project"]);

  if (map.find("IP_FTP") != map.end())
    ipFTP = (map["IP_FTP"]);
  else
    ipFTP = ipMysql;
}

synPathListPtr coreSet::getSynDir() {
  auto db = coreSql::getCoreSql().getConnection();
  doodle::Synfile table{};
  doodle::Episodes epTable{};

  nlohmann::json root;

  synPathListPtr list;

  for (auto &&row :
       db->run(sqlpp::select(table.path)
                   .from(table.join(epTable).on(table.episodesId == epTable.id))
                   .where(epTable.episodes == syneps and
                          epTable.projectId == project.first))) {
    dstring str = row.path;
    try {
      root = nlohmann::json::parse(str);
    } catch (nlohmann::json::parse_error &err) {
      DOODLE_LOG_INFO(err.what() << " "
                                 << "解析同步目录失败");
    }
    for (const auto &item : root) {
      synPath_struct fileSyn{};
      fileSyn.local  = item.value("Right", "");
      fileSyn.server = item.value("Left", "");
      list.push_back(fileSyn);
    }
    DOODLE_LOG_INFO(row.path);
  }
  return list;
}

void coreSet::getCacheDiskPath() {
  for (QStorageInfo &x : QStorageInfo::mountedVolumes()) {
    if (x.isValid() && x.isReady()) {
      if (!x.isReadOnly()) {
        if (((double)x.bytesAvailable() / (double)x.bytesTotal() > 0.5f) &&
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
}
dpath coreSet::program_location() {
  return boost::filesystem::path{boost::dll::program_location()}.parent_path();
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
      boost::algorithm::to_lower_copy(dopinyin::convert{}.toEn(user_str));
  db->run(sqlpp::insert_into(table).set(table.user     = user_str,
                                        table.password = pow));
  return true;
}
CORE_NAMESPACE_E
