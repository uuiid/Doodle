#include "coreset.h"
#include <corelib/core/coresql.h>
#include <corelib/core/Project.h>
#include <pinyinlib/convert.h>
#include <loggerlib/Logger.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/sqlite3/sqlite3.h>

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <fstream>

#include <corelib/filesystem/FileSystem.h>
#include <corelib/core/Project.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/process.hpp>
#include <boost/dll.hpp>
#include <boost/dll/runtime_symbol_info.hpp>

#include <magic_enum.hpp>
#include <ShlObj.h>
DOODLE_NAMESPACE_S

const dstring coreSet::settingFileName = "doodle_conf.json";

coreSet &coreSet::getSet() {
  static coreSet install;
  return install;
}

void coreSet::init() {
  //这里我们手动做一些工作
  //获取环境变量
  PWSTR pManager;
  SHGetKnownFolderPath(FOLDERID_Documents, NULL, NULL, &pManager);
  if (!pManager) std::runtime_error("无法找到保存路径");

  doc = fileSys::path{pManager} / "doodle";
  getSetting();

  getCacheDiskPath();
  appendEnvironment();

  if (!boost::filesystem::exists(getCacheRoot())) {
    boost::filesystem::create_directories(getCacheRoot());
  }
}

void coreSet::reInit() {
}

void coreSet::initdb() {
  coreSql &sql = coreSql::getCoreSql();
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

  root["user"]       = user;
  root["department"] = getDepartment();
  root["synPath"]    = synPath.generic_string();

  boost::filesystem::ofstream outjosn;
  outjosn.open(doc / settingFileName, std::ifstream::binary);
  outjosn << root.dump();
  outjosn.close();
}

coreSet::coreSet()
    : user("user"),
      department(Department::VFX),
      synPath("D:/ue_prj"),
      cacheRoot("C:/Doodle_cache"),
      doc("C:/Doodle_cache") {
}

void coreSet::getSetting() {
  if (boost::filesystem::exists(doc / settingFileName)) {
    fileSys::path strFile(doc / settingFileName);
    boost::filesystem::ifstream inJosn;
    inJosn.open(doc / settingFileName, std::ifstream::binary);

    nlohmann::json root = nlohmann::json::parse(inJosn);
    inJosn.close();

    user       = root.value("user", "uset_tmp");
    auto k_dep = root.value("department", "VFX");
    department = magic_enum::enum_cast<Department>(k_dep).value_or(Department::VFX);
    synPath    = root.value("synPath", "D:/ue_prj");
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

std::shared_ptr<Project> coreSet::getProject() {
  return p_projects[""];
}

std::vector<std::string> coreSet::getAllProjectNames() {
  std::vector<std::string> list{};
  for (auto &&it : p_projects) {
    auto pr = it.second->Name();
    list.emplace_back(std::move(pr));
  }
  return list;
}

dstring coreSet::getUser() const { return user; }

dstring coreSet::getUser_en() const {
  return boost::algorithm::to_lower_copy(
      dopinyin::convert::Get().toEn(user));
}

void coreSet::setUser(const dstring &value) { user = value; }

fileSys::path coreSet::getDoc() const { return doc; }

fileSys::path coreSet::getCacheRoot() const { return cacheRoot; }

void coreSet::getServerSetting() {
  //获得项目个数

  // *shotRoot   = map["shotRoot"];
  // *assRoot    = map["assetsRoot"];
  // *prjectRoot = map["project"];
}

void coreSet::getCacheDiskPath() {
  const static std::vector<std::string> dirs{"D:/",
                                             "E:/",
                                             "F:/",
                                             "G:/",
                                             "H:/",
                                             "I:/",
                                             "J:/",
                                             "K:/",
                                             "L:/"};
  for (auto &dir : dirs) {
    if (fileSys::exists(dir)) {
      auto info = fileSys::space(dir);
      if (((float)info.available / (float)info.available) > 0.05) {
        cacheRoot = dir + "Doodle_cache";
        break;
      }
    }
  }
}

const fileSys::path coreSet::getSynPathLocale() const { return synPath; }

void coreSet::setSynPathLocale(const fileSys::path &syn_path) {
  synPath = syn_path;
}

fileSys::path coreSet::program_location() {
  return boost::dll::program_location().parent_path();
}
fileSys::path coreSet::program_location(const fileSys::path &path) {
  return program_location() / path;
}

DOODLE_NAMESPACE_E
