#include <corelib/core/coreset.h>

#include <corelib/core/coresql.h>

#include <corelib/Exception/Exception.h>
#include <pinyinlib/convert.h>
#include <loggerlib/Logger.h>

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/process.hpp>
#include <boost/dll.hpp>
#include <boost/dll/runtime_symbol_info.hpp>

#include <magic_enum.hpp>
#include <ShlObj.h>
#include <cereal/cereal.hpp>
#include <cereal/archives/portable_binary.hpp>

DOODLE_NAMESPACE_S

const std::string coreSet::settingFileName = "doodle_config.bin";

coreSet &coreSet::getSet() {
  static coreSet install;
  return install;
}

void coreSet::init() {
  //这里我们手动做一些工作
  //获取环境变量
  PWSTR pManager;
  SHGetKnownFolderPath(FOLDERID_Documents, NULL, NULL, &pManager);
  if (!pManager) DoodleError("无法找到保存路径");

  doc = FSys::path{pManager} / "doodle";
  getSetting();
  coreSql &sql = coreSql::getCoreSql();
  getCacheDiskPath();
  appendEnvironment();

  if (!boost::filesystem::exists(getCacheRoot())) {
    boost::filesystem::create_directories(getCacheRoot());
  }
}

void coreSet::reInit() {
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
  if (ue4_setting.hasPath() && !FSys::exists(ue4_setting.Path() / DOODLE_UE_PATH)) {
    ue4_setting.setPath({});
    throw FileError{ue4_setting.Path(), " 在路径中没有找到ue,不保存"};
  }
  boost::filesystem::ofstream outjosn{doc / settingFileName, std::ifstream::binary};
  cereal::PortableBinaryOutputArchive out{outjosn};
  out(*this);
}

void coreSet::getSetting() {
  static FSys::path k_settingFileName = doc / settingFileName;
  if (boost::filesystem::exists(k_settingFileName)) {
    FSys::path strFile(k_settingFileName);
    boost::filesystem::ifstream inJosn{k_settingFileName, std::ifstream::binary};

    cereal::PortableBinaryInputArchive incereal{inJosn};
    incereal(*this);
  }
}
coreSet::coreSet()
    : user("user"),
      department(Department::VFX),
      cacheRoot("C:/Doodle_cache"),
      doc("C:/Doodle_cache"),
      p_uuid_gen(),
      ue4_setting(Ue4Setting::Get()),
      p_project_list(),
      p_project() {
}

std::string coreSet::toIpPath(const std::string &path) {
  static boost::regex exp("^[A-Z]:");
  if (boost::regex_search(path, exp)) {
    return path.substr(2);
  }
  return path;
}

boost::uuids::uuid coreSet::getUUID() {
  return p_uuid_gen();
}

std::string coreSet::getDepartment() const {
  return std::string{magic_enum::enum_name(department)};
}

const Department &coreSet::getDepartmentEnum() const {
  return department;
}

void coreSet::setDepartment(const std::string &value) {
  department = magic_enum::enum_cast<Department>(value).value_or(Department::VFX);
}

std::string coreSet::getUser() const { return user; }

std::string coreSet::getUser_en() const {
  return boost::algorithm::to_lower_copy(
      dopinyin::convert::Get().toEn(user));
}

void coreSet::setUser(const std::string &value) { user = value; }

FSys::path coreSet::getDoc() const { return doc; }

FSys::path coreSet::getCacheRoot() const { return cacheRoot; }

bool coreSet::hasProject() {
  return !p_project_list.empty();
}

std::vector<ProjectPtr> coreSet::getAllProjects() const {
  return p_project_list;
}
void coreSet::installProject(const ProjectPtr &Project_) {
  p_project_list.emplace_back(Project_);
}

const ProjectPtr &coreSet::Project_() const {
  if (!p_project)
    throw nullptr_error{"没有项目"};
  return p_project;
}

void coreSet::setProject_(const ProjectPtr &Project_) {
  p_project = Project_;
  auto it   = std::find(p_project_list.begin(), p_project_list.end(), Project_);
  if (it == p_project_list.end()) {
    p_project_list.emplace_back(Project_);
  }
}

void coreSet::setProject_(const Project *Project_) {
  auto it = std::find_if(p_project_list.begin(), p_project_list.end(),
                         [Project_](ProjectPtr &prj) { return Project_ == prj.get(); });
  if (it != p_project_list.end()) {
    p_project = *it;
  } else {
    throw DoodleError{"无法找到项目"};
  }
}

void coreSet::deleteProject(const Project *Project_) {
  auto it = std::find_if(p_project_list.begin(), p_project_list.end(),
                         [Project_](ProjectPtr &prj) { return Project_ == prj.get(); });
  if (it != p_project_list.end()) {
    p_project_list.erase(it);
  } else {
    throw DoodleError{"无法找到项目"};
  }
}

int coreSet::getProjectIndex() const {
  auto it    = std::find(p_project_list.begin(), p_project_list.end(), p_project);
  auto index = std::distance(p_project_list.begin(), it);
  return boost::numeric_cast<int>(index);
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
    if (FSys::exists(dir)) {
      auto info = FSys::space(dir);
      if (((float)info.available / (float)info.available) > 0.05) {
        cacheRoot = dir + "Doodle_cache";
        break;
      }
    }
  }
}

FSys::path coreSet::program_location() {
  return boost::dll::program_location().parent_path();
}
FSys::path coreSet::program_location(const FSys::path &path) {
  return program_location() / path;
}

DOODLE_NAMESPACE_E
