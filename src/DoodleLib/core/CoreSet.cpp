#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/Logger/Logger.h>
#include <DoodleLib/PinYin/convert.h>
#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/core/CoreSql.h>
#include <DoodleLib/core/static_value.h>
#include <DoodleLib/rpc/RpcFileSystemClient.h>
#include <DoodleLib/rpc/RpcMetadataClient.h>
#include <Metadata/MetadataFactory.h>
#include <ShlObj.h>
#include <date/tz.h>
#include <google/protobuf/service.h>
#include <grpcpp/grpcpp.h>
#include <sqlpp11/mysql/mysql.h>

#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <magic_enum.hpp>
#include <nlohmann/json.hpp>

namespace doodle {

CoreSet &CoreSet::getSet() {
  static CoreSet install;
  return install;
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
  p_ue4_setting.testValue();
  if (p_ue4_setting.hasPath() && !FSys::exists(p_ue4_setting.Path() / staticValue::ue_path_obj())) {
    p_ue4_setting.setPath({});
    throw FileError{p_ue4_setting.Path(), " 在路径中没有找到ue,不保存"};
  }
  if (!FSys::exists(p_mayaPath / "maya.exe")) {
    throw FileError{p_mayaPath, " 在路径中没有找到maya,不保存"};
  }

  FSys::ofstream outjosn{p_doc / configFileName(), std::ios::out | std::ios::binary};
  cereal::PortableBinaryOutputArchive out{outjosn};
  out(*this);
}

void CoreSet::getSetting() {
  static FSys::path k_settingFileName = p_doc / configFileName();
  if (FSys::exists(k_settingFileName)) {
    FSys::path strFile(k_settingFileName);
    FSys::ifstream inJosn{k_settingFileName, std::ifstream::binary};

    cereal::PortableBinaryInputArchive incereal{inJosn};
    try {
      incereal(*this);
    } catch (const cereal::Exception &err) {
      DOODLE_LOG_DEBUG(err.what());
    }
  }
}
CoreSet::CoreSet()
    : p_user_("user"),
      p_department_(Department::None_),
      p_cache_root("C:/Doodle/cache"),
      p_doc("C:/Doodle/doc"),
      p_data_root("C:/Doodle/data"),
      p_uuid_gen(),
      p_ue4_setting(Ue4Setting::Get()),
      p_mayaPath(),
#ifdef NDEBUG
      p_server_host("192.168.20.60"),
#else
      p_server_host("192.168.20.59"),
#endif
      p_sql_port(3306),
      p_meta_rpc_port(60999),
      p_file_rpc_port(60998),
      p_sql_host("192.168.20.60"),
      p_sql_user("deve"),
      p_sql_password("deve") {
  ///这里我们手动做一些工作
  ///获取环境变量 FOLDERID_Documents
  PWSTR pManager;
  SHGetKnownFolderPath(FOLDERID_Documents, NULL, nullptr, &pManager);
  if (!pManager)
    throw DoodleError("无法找到保存路径");

  p_doc = FSys::path{pManager} / "doodle";
  CoTaskMemFree(pManager);

  getCacheDiskPath();
  p_data_root = p_cache_root.parent_path() / "data";

  if (!FSys::exists(p_doc))
    FSys::create_directories(p_doc);
  if (!FSys::exists(getCacheRoot())) {
    FSys::create_directories(getCacheRoot());
  }
  if (!FSys::exists(getDataRoot())) {
    FSys::create_directories(getDataRoot());
  }
  /// 在这里我们初始化date tz 时区数据库
  date::set_install(program_location("tzdata").generic_string());
  DOODLE_LOG_INFO("初始化时区数据库: {}", program_location("tzdata").generic_string());
  findMaya();
  getSetting();
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
  return std::string{magic_enum::enum_name(p_department_)};
}

const Department &CoreSet::getDepartmentEnum() const {
  return p_department_;
}

void CoreSet::setDepartment(const std::string &value) {
  p_department_ = magic_enum::enum_cast<Department>(value).value_or(Department::None_);
}

std::string CoreSet::getUser() const { return p_user_; }

std::string CoreSet::getUser_en() const {
  return boost::algorithm::to_lower_copy(
      convert::Get().toEn(p_user_));
}

void CoreSet::setUser(const std::string &value) {
  p_user_ = value;
}

FSys::path CoreSet::getDoc() const { return p_doc; }

FSys::path CoreSet::getCacheRoot() const {
  return p_cache_root;
}

FSys::path CoreSet::getCacheRoot(const FSys::path &in_path) const {
  auto path = p_cache_root / in_path;
  if (!FSys::exists(path))
    FSys::create_directories(path);
  return path;
}

void CoreSet::setCacheRoot(const FSys::path &path) {
  p_cache_root = path;
}

FSys::path CoreSet::getDataRoot() const {
  return p_data_root;
}

void CoreSet::setDataRoot(const FSys::path &in_path) {
  p_data_root = in_path;
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
        p_cache_root = dir + "Doodle/cache";
        break;
      }
    }
  }
}

FSys::path CoreSet::program_location() {
  return FSys::current_path();
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

int CoreSet::getSqlPort() const {
  return p_sql_port;
}
void CoreSet::setSqlPort(int in_sqlPort) {
  p_sql_port = in_sqlPort;
}
const std::string &CoreSet::getSqlHost() const {
  return p_sql_host;
}
void CoreSet::setSqlHost(const std::string &in_sqlHost) {
  p_sql_host = in_sqlHost;
}
const std::string &CoreSet::getSqlUser() const {
  return p_sql_user;
}
void CoreSet::setSqlUser(const std::string &in_sqlUser) {
  p_sql_user = in_sqlUser;
}
const std::string &CoreSet::getSqlPassword() const {
  return p_sql_password;
}
void CoreSet::setSqlPassword(const std::string &in_sqlPassword) {
  p_sql_password = in_sqlPassword;
}
int CoreSet::getMetaRpcPort() const {
  return p_meta_rpc_port;
}
void CoreSet::setMetaRpcPort(int in_metaRpcPort) {
  p_meta_rpc_port = in_metaRpcPort;
}
int CoreSet::getFileRpcPort() const {
  return p_file_rpc_port;
}
void CoreSet::setFileRpcPort(int in_fileRpcPort) {
  p_file_rpc_port = in_fileRpcPort;
}
std::string CoreSet::get_server_host() {
  return p_server_host;
}

}  // namespace doodle
