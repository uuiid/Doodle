#include <DoodleLib/core/coreset.h>
#include <Exception/Exception.h>
#include <PinYin/convert.h>

#include <nlohmann/json.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/process.hpp>
#include <boost/dll.hpp>

#include <magic_enum.hpp>
#include <ShlObj.h>

#include <cereal/archives/portable_binary.hpp>

DOODLE_NAMESPACE_S

coreSet &coreSet::getSet() {
  static coreSet install;
  return install;
}

void coreSet::init() {
  //这里我们手动做一些工作
  //获取环境变量

  PWSTR pManager;
  SHGetKnownFolderPath(FOLDERID_Documents, NULL, nullptr, &pManager);
  if (!pManager) throw DoodleError("无法找到保存路径");

  doc = FSys::path{pManager} / "doodle";
  if (!FSys::exists(doc))
    FSys::create_directories(doc);
  findMaya();
  getSetting();

  getCacheDiskPath();

  if (!FSys::exists(getCacheRoot())) {
    FSys::create_directories(getCacheRoot());
  }
}

void coreSet::reInit() {
}

void coreSet::findMaya() {
  if (FSys::exists(R"(C:\Program Files\Autodesk\Maya2020\bin)")) {
    p_mayaPath = R"(C:\Program Files\Autodesk\Maya2020\bin\)";
  } else if (FSys::exists(R"(C:\Program Files\Autodesk\Maya2019\bin)")) {
    p_mayaPath = R"(C:\Program Files\Autodesk\Maya2019\bin\)";
  } else if (FSys::exists(R"(C:\Program Files\Autodesk\Maya2018\bin)")) {
    p_mayaPath = R"(C:\Program Files\Autodesk\Maya2018\bin\)";
  }
}

bool coreSet::hasMaya() const noexcept {
  return !p_mayaPath.empty();
}

const FSys::path &coreSet::MayaPath() const noexcept {
  return p_mayaPath;
}

void coreSet::setMayaPath(const FSys::path &in_MayaPath) noexcept {
  p_mayaPath = in_MayaPath;
}

void coreSet::writeDoodleLocalSet() {
  ue4_setting.testValue();
  if (ue4_setting.hasPath() && !FSys::exists(ue4_setting.Path() / DOODLE_UE_PATH)) {
    ue4_setting.setPath({});
    throw FileError{ue4_setting.Path(), " 在路径中没有找到ue,不保存"};
  }
  if (!FSys::exists(p_mayaPath / "maya.exe")) {
    throw FileError{p_mayaPath, " 在路径中没有找到maya,不保存"};
  }

  FSys::ofstream outjosn{doc / configFileName(), std::ifstream::binary};
  cereal::PortableBinaryOutputArchive out{outjosn};
  out(*this);
}

void coreSet::getSetting() {
  static FSys::path k_settingFileName = doc / configFileName();
  if (FSys::exists(k_settingFileName)) {
    FSys::path strFile(k_settingFileName);
    FSys::ifstream inJosn{k_settingFileName, std::ifstream::binary};

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
      p_matadata_setting_(MetadataSet::Get()),
      p_project_list(),
      p_project(),
      p_mayaPath() {
}

FSys::path coreSet::toIpPath(const FSys::path &path) {
  std::wstring str{};
  str.resize(MAX_PATH);
  DWORD dwResult, cchBuff = str.size();
  dwResult = WNetGetConnection(path.generic_wstring().c_str(), str.data(), &cchBuff);
  switch (dwResult) {
    case NO_ERROR:
      break;
    case ERROR_BUFFER_OVERFLOW: {
      str.resize(cchBuff);
      dwResult = WNetGetConnection(path.generic_wstring().c_str(), str.data(), &cchBuff);
      break;
    }
    default: {
      throw DoodleError{"错误代码：" + std::to_string(dwResult)};
    } break;
  }
  return {str};
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
      convert::Get().toEn(user));
}

void coreSet::setUser(const std::string &value) {
  user = value;
}

FSys::path coreSet::getDoc() const { return doc; }

FSys::path coreSet::getCacheRoot() const {
  return cacheRoot;
}

FSys::path coreSet::getCacheRoot(const FSys::path &in_path) const {
  auto path = cacheRoot / in_path;
  if (!FSys::exists(path))
    FSys::create_directories(path);
  return path;
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
  return {boost::dll::program_location().parent_path().generic_string()};
}
FSys::path coreSet::program_location(const FSys::path &path) {
  return program_location() / path;
}
std::string coreSet::configFileName() {
  static std::string str{"doodle_config.bin"};
  return str;
}
std::string coreSet::getUUIDStr() {
  return boost::uuids::to_string(getUUID());
}
void coreSet::hideFolder(const FSys::path &path) {
  auto attr = GetFileAttributes(path.generic_wstring().c_str());
  if ((attr & FILE_ATTRIBUTE_HIDDEN) == 0) {
    SetFileAttributes(path.generic_wstring().c_str(), attr | FILE_ATTRIBUTE_HIDDEN);
  }
}

DOODLE_NAMESPACE_E
