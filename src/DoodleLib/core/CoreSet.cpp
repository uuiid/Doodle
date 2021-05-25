#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/PinYin/convert.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/core/CoreSql.h>
#include <ShlObj.h>

#include <boost/algorithm/string.hpp>
#include <boost/dll.hpp>
#include <boost/process.hpp>
#include <boost/regex.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <magic_enum.hpp>
#include <nlohmann/json.hpp>

DOODLE_NAMESPACE_S

CoreSet &CoreSet::getSet() {
  static CoreSet install;
  return install;
}

void CoreSet::init() {
  ///这里我们手动做一些工作
  ///获取环境变量 FOLDERID_Documents
  PWSTR pManager;
  SHGetKnownFolderPath(FOLDERID_Documents, NULL, nullptr, &pManager);
  if (!pManager) throw DoodleError("无法找到保存路径");

  doc = FSys::path{pManager} / "doodle";
  CoTaskMemFree(pManager);

  if (!FSys::exists(doc))
    FSys::create_directories(doc);
  findMaya();
  getSetting();

  getCacheDiskPath();

  if (!FSys::exists(getCacheRoot())) {
    FSys::create_directories(getCacheRoot());
  }

  ///触发一次 CoreSql 初始化并测试数据库连接
  auto &k_sql = CoreSql::Get();
  auto test_sql = k_sql.getConnection();
}

void CoreSet::reInit() {
}

void CoreSet::findMaya() {
  if (FSys::exists(R"(C:\Program Files\Autodesk\Maya2020\bin)")) {
    p_mayaPath = R"(C:\Program Files\Autodesk\Maya2020\bin\)";
  } else if (FSys::exists(R"(C:\Program Files\Autodesk\Maya2019\bin)")) {
    p_mayaPath = R"(C:\Program Files\Autodesk\Maya2019\bin\)";
  } else if (FSys::exists(R"(C:\Program Files\Autodesk\Maya2018\bin)")) {
    p_mayaPath = R"(C:\Program Files\Autodesk\Maya2018\bin\)";
  }
}

bool CoreSet::hasMaya() const noexcept {
  return !p_mayaPath.empty();
}

const FSys::path &CoreSet::MayaPath() const noexcept {
  return p_mayaPath;
}

void CoreSet::setMayaPath(const FSys::path &in_MayaPath) noexcept {
  p_mayaPath = in_MayaPath;
}

void CoreSet::writeDoodleLocalSet() {
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

void CoreSet::getSetting() {
  static FSys::path k_settingFileName = doc / configFileName();
  if (FSys::exists(k_settingFileName)) {
    FSys::path strFile(k_settingFileName);
    FSys::ifstream inJosn{k_settingFileName, std::ifstream::binary};

    cereal::PortableBinaryInputArchive incereal{inJosn};
    incereal(*this);
  }
}
CoreSet::CoreSet()
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

FSys::path CoreSet::toIpPath(const FSys::path &path) {
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

boost::uuids::uuid CoreSet::getUUID() {
  return p_uuid_gen();
}

std::string CoreSet::getDepartment() const {
  return std::string{magic_enum::enum_name(department)};
}

const Department &CoreSet::getDepartmentEnum() const {
  return department;
}

void CoreSet::setDepartment(const std::string &value) {
  department = magic_enum::enum_cast<Department>(value).value_or(Department::VFX);
}

std::string CoreSet::getUser() const { return user; }

std::string CoreSet::getUser_en() const {
  return boost::algorithm::to_lower_copy(
      convert::Get().toEn(user));
}

void CoreSet::setUser(const std::string &value) {
  user = value;
}

FSys::path CoreSet::getDoc() const { return doc; }

FSys::path CoreSet::getCacheRoot() const {
  return cacheRoot;
}

FSys::path CoreSet::getCacheRoot(const FSys::path &in_path) const {
  auto path = cacheRoot / in_path;
  if (!FSys::exists(path))
    FSys::create_directories(path);
  return path;
}

void CoreSet::getCacheDiskPath() {
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

FSys::path CoreSet::program_location() {
  return {boost::dll::program_location().parent_path().generic_string()};
}
FSys::path CoreSet::program_location(const FSys::path &path) {
  return program_location() / path;
}
std::string CoreSet::configFileName() {
  static std::string str{"doodle_config.bin"};
  return str;
}
std::string CoreSet::getUUIDStr() {
  return boost::uuids::to_string(getUUID());
}
void CoreSet::hideFolder(const FSys::path &path) {
  auto attr = GetFileAttributes(path.generic_wstring().c_str());
  if ((attr & FILE_ATTRIBUTE_HIDDEN) == 0) {
    SetFileAttributes(path.generic_wstring().c_str(), attr | FILE_ATTRIBUTE_HIDDEN);
  }
}

DOODLE_NAMESPACE_E
