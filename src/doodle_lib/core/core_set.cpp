#include <Metadata/metadata_factory.h>
#include <date/tz.h>
#include <doodle_lib/Exception/exception.h>
#include <doodle_lib/Logger/logger.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/core_sql.h>
#include <doodle_lib/core/static_value.h>
#include <doodle_lib/pin_yin/convert.h>
#include <doodle_lib/rpc/rpc_file_system_client.h>
#include <doodle_lib/rpc/rpc_metadata_client.h>
#include <google/protobuf/service.h>
#include <grpcpp/grpcpp.h>
#include <sqlpp11/mysql/mysql.h>

#include <boost/algorithm/string.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/process.hpp>
#include <magic_enum.hpp>
#include <nlohmann/json.hpp>
#ifdef _WIN32
#include <ShlObj.h>
#else
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif  // _WIN32

BOOST_CLASS_EXPORT_IMPLEMENT(doodle::core_set)

namespace doodle {

FSys::path get_pwd()
#ifdef _WIN32
{
  ///这里我们手动做一些工作
  ///获取环境变量 FOLDERID_Documents
  PWSTR pManager;
  SHGetKnownFolderPath(FOLDERID_Documents, NULL, nullptr, &pManager);
  if (!pManager) {
    std::cout << "unable to find a save path" << std::endl;
    throw doodle_error("无法找到保存路径");
  }

  auto k_path = FSys::path{pManager};
  CoTaskMemFree(pManager);
  return k_path;
}
#else
{
  auto pw = getpwuid(getuid())->pw_dir;
  return FSys::path{pw};
};
#endif  // _WIN32

core_set &core_set::getSet() {
  static core_set install;
  return install;
}

void core_set::findMaya() {
  if (FSys::exists(R"(C:\Program Files\Autodesk\Maya2020\bin)")) {
    p_mayaPath = R"(C:\Program Files\Autodesk\Maya2020\bin\)";
  } else if (FSys::exists(R"(C:\Program Files\Autodesk\Maya2019\bin)")) {
    p_mayaPath = R"(C:\Program Files\Autodesk\Maya2019\bin\)";
  } else if (FSys::exists(R"(C:\Program Files\Autodesk\Maya2018\bin)")) {
    p_mayaPath = R"(C:\Program Files\Autodesk\Maya2018\bin\)";
  }
}

bool core_set::has_maya() const noexcept {
  return !p_mayaPath.empty();
}

const FSys::path &core_set::maya_path() const noexcept {
  return p_mayaPath;
}

void core_set::set_maya_path(const FSys::path &in_MayaPath) noexcept {
  p_mayaPath = in_MayaPath;
}

void core_set::write_doodle_local_set() {
  p_ue4_setting.test_value();
  if (p_ue4_setting.has_path() && !FSys::exists(p_ue4_setting.get_path() / staticValue::ue_path_obj())) {
    p_ue4_setting.set_path({});
    throw file_error{p_ue4_setting.get_path(), " 在路径中没有找到ue,不保存"};
  }
  if (!FSys::exists(p_mayaPath / "maya.exe")) {
    throw file_error{p_mayaPath, " 在路径中没有找到maya,不保存"};
  }

  FSys::ofstream outjosn{p_doc / config_file_name(), std::ios::out | std::ios::binary};
  boost::archive::text_oarchive out{outjosn};
  out << *this;
}

void core_set::get_setting() {
  static FSys::path k_settingFileName = get_doc() / config_file_name();
  if (FSys::exists(k_settingFileName)) {
    FSys::path strFile(k_settingFileName);
    FSys::ifstream inJosn{k_settingFileName, std::ifstream::binary};

    boost::archive::text_iarchive out{inJosn};
    try {
      out >> *this;
    } catch (const boost::archive::archive_exception &err) {
      DOODLE_LOG_DEBUG(err.what());
    }
  }
}
core_set::core_set()
    : p_user_("user"),
      p_department_(department::None_),
      p_doc(),
      p_uuid_gen(),
      p_ue4_setting(ue4_setting::Get()),
      p_mayaPath(),
#ifdef NDEBUG
      p_server_host("rpc.server.doodle.com"),
#else
      p_server_host("dev.rpc.server.doodle.com"),
#endif
      p_sql_port(3306),
      p_meta_rpc_port(60999),
      p_file_rpc_port(60998),
#ifdef NDEBUG
      p_sql_host("mysql.server.doodle.com"),
#else
      p_sql_host("dev.mysql.server.doodle.com"),
#endif
      p_sql_user("deve"),
      p_sql_password("deve"),
      p_max_thread(std::thread::hardware_concurrency() - 2),
      p_stop(false),
      p_mutex(),
      p_condition(),
      p_root(FSys::temp_directory_path() / "Doodle") {
  p_doc = get_pwd() / "doodle";

  get_cache_disk_path();

  if (!FSys::exists(p_doc))
    FSys::create_directories(p_doc);
  if (!FSys::exists(get_cache_root())) {
    FSys::create_directories(get_cache_root());
  }
  if (!FSys::exists(get_data_root())) {
    FSys::create_directories(get_data_root());
  }
  findMaya();
  if (p_doc.empty())
    p_doc = get_root() / "doc";

  get_setting();
}

boost::uuids::uuid core_set::get_uuid() {
  return p_uuid_gen();
}

std::string core_set::get_department() const {
  return std::string{magic_enum::enum_name(p_department_)};
}

const department &core_set::get_department_enum() const {
  return p_department_;
}

void core_set::set_department(const std::string &value) {
  p_department_ = magic_enum::enum_cast<department>(value).value_or(department::None_);
}

std::string core_set::get_user() const { return p_user_; }

std::string core_set::get_user_en() const {
  return boost::algorithm::to_lower_copy(
      convert::Get().toEn(p_user_));
}

void core_set::set_user(const std::string &value) {
  p_user_ = value;
}

FSys::path core_set::get_doc() const {
  return p_doc;
}
FSys::path core_set::get_config_file() const {
  return p_doc / config_file_name();
}

FSys::path core_set::get_root() const {
  return p_root;
}
void core_set::set_root(const FSys::path &in_path) {
  p_root      = in_path;
  _root_cache = p_root / "cache";
  _root_data  = p_root / "data";
}

FSys::path core_set::get_cache_root() const {
  return _root_cache;
}

FSys::path core_set::get_cache_root(const FSys::path &in_path) const {
  auto path = get_cache_root() / in_path;
  if (!FSys::exists(path))
    FSys::create_directories(path);
  return path;
}

FSys::path core_set::get_data_root() const {
  return _root_data;
}

void core_set::get_cache_disk_path() {
  const static std::vector<FSys::path> dirs{"D:/",
                                            "E:/",
                                            "F:/",
                                            "G:/",
                                            "H:/",
                                            "I:/",
                                            "J:/",
                                            "K:/",
                                            "L:/"};
  for (auto &dir : dirs) {
    try {
      if (FSys::exists(dir)) {
        auto info = FSys::space(dir);
        if (((float)info.available / (float)info.capacity) > 0.2) {
          set_root(dir / "Doodle");
          break;
        }
      }
    } catch (const FSys::filesystem_error &e) {
      std::cout << e.what() << std::endl;
    }
  }
}

FSys::path core_set::program_location() {
  return FSys::current_path();
}
FSys::path core_set::program_location(const FSys::path &path) {
  return program_location() / path;
}
std::string core_set::config_file_name() {
  static std::string str{"doodle_config.bin"};
  return str;
}
std::string core_set::get_uuid_str() {
  return boost::uuids::to_string(get_uuid());
}

int core_set::get_sql_port() const {
  return p_sql_port;
}
void core_set::set_sql_port(int in_sqlPort) {
  p_sql_port = in_sqlPort;
}
const std::string &core_set::get_sql_host() const {
  return p_sql_host;
}
void core_set::set_sql_host(const std::string &in_sqlHost) {
  p_sql_host = in_sqlHost;
}
const std::string &core_set::get_sql_user() const {
  return p_sql_user;
}
void core_set::set_sql_user(const std::string &in_sqlUser) {
  p_sql_user = in_sqlUser;
}
const std::string &core_set::get_sql_password() const {
  return p_sql_password;
}
void core_set::set_sql_password(const std::string &in_sqlPassword) {
  p_sql_password = in_sqlPassword;
}
int core_set::get_meta_rpc_port() const {
  return p_meta_rpc_port;
}
void core_set::set_meta_rpc_port(int in_metaRpcPort) {
  p_meta_rpc_port = in_metaRpcPort;
}
int core_set::get_file_rpc_port() const {
  return p_file_rpc_port;
}
void core_set::set_file_rpc_port(int in_fileRpcPort) {
  p_file_rpc_port = in_fileRpcPort;
}
std::string core_set::get_server_host() {
  return p_server_host;
}
void core_set::from_json(const nlohmann::json &nlohmann_json_j) {
  nlohmann_json_j.at("p_sql_port").get_to(p_sql_port);
  nlohmann_json_j.at("p_meta_rpc_port").get_to(p_meta_rpc_port);
  nlohmann_json_j.at("p_file_rpc_port").get_to(p_file_rpc_port);
  nlohmann_json_j.at("p_sql_host").get_to(p_sql_host);
  nlohmann_json_j.at("p_sql_user").get_to(p_sql_user);
  nlohmann_json_j.at("p_sql_password").get_to(p_sql_password);
}

void core_set::set_max_tread(const std::uint16_t in) {
  p_max_thread = in;
}
void core_set::set_department(const department &value) {
  p_department_ = value;
}

}  // namespace doodle
