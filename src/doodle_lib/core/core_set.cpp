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

FSys::path win::get_pwd()
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

FSys::path win::get_font() {
  ///这里我们手动做一些工作
  ///获取环境变量 FOLDERID_Documents
  PWSTR pManager;
  SHGetKnownFolderPath(FOLDERID_Fonts, NULL, nullptr, &pManager);
  if (!pManager) {
    std::cout << "unable to find a save path" << std::endl;
    throw doodle_error("无法找到保存路径");
  }

  auto k_path = FSys::path{pManager};
  CoTaskMemFree(pManager);
  return k_path;
}

core_set &core_set::getSet() {
  static core_set install;
  return install;
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

core_set::core_set()
    : p_user_("user"),
      p_department_(department::None_),
      p_doc(FSys::current_path()),
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
      p_root(FSys::temp_directory_path() / "Doodle"),
      _root_cache(p_root / "cache"),
      _root_data(p_root / "data"),
      timeout(3600) {
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

void core_set::set_server_host(const string &in_host) {
  p_server_host = in_host;
}

FSys::path core_set::get_root() const {
  return p_root;
}
void core_set::set_root(const FSys::path &in_path) {
  p_root      = in_path;
  _root_cache = p_root / "cache";
  _root_data  = p_root / "data";
  DOODLE_LOG_INFO("设置缓存目录", p_root);
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

void core_set::set_max_tread(const std::uint16_t in) {
  p_max_thread = in;
}
void core_set::set_department(const department &value) {
  p_department_ = value;
}

core_set_init::core_set_init()
    : p_set(core_set::getSet()) {
  if (!FSys::exists(p_set.p_doc))
    FSys::create_directories(p_set.p_doc);
  if (!FSys::exists(p_set.get_cache_root())) {
    FSys::create_directories(p_set.get_cache_root());
  }
  if (!FSys::exists(p_set.get_data_root())) {
    FSys::create_directories(p_set.get_data_root());
  }
}
bool core_set_init::find_maya() {
  DOODLE_LOG_INFO("寻找maya");

  if (FSys::exists(R"(C:\Program Files\Autodesk\Maya2020\bin)")) {
    p_set.p_mayaPath = R"(C:\Program Files\Autodesk\Maya2020\bin\)";
    return true;
  } else if (FSys::exists(R"(C:\Program Files\Autodesk\Maya2019\bin)")) {
    p_set.p_mayaPath = R"(C:\Program Files\Autodesk\Maya2019\bin\)";
    return true;
  } else if (FSys::exists(R"(C:\Program Files\Autodesk\Maya2018\bin)")) {
    p_set.p_mayaPath = R"(C:\Program Files\Autodesk\Maya2018\bin\)";
    return true;
  }
  return false;
}

bool core_set_init::read_file() {
  FSys::path l_k_setting_file_name = p_set.get_doc() / p_set.config_file_name();
  DOODLE_LOG_INFO("读取配置文件 {}", l_k_setting_file_name);
  if (FSys::exists(l_k_setting_file_name)) {
    FSys::path l_str_file(l_k_setting_file_name);
    FSys::ifstream l_in_josn{l_k_setting_file_name, std::ifstream::binary};

    boost::archive::text_iarchive l_out{l_in_josn};
    try {
      l_out >> p_set;
    } catch (const boost::archive::archive_exception &err) {
      DOODLE_LOG_DEBUG(err.what());
      return false;
    }
    return true;
  }
  return false;
}
bool core_set_init::write_file() {
  DOODLE_LOG_INFO("写入配置文件 {}", p_set.p_doc / p_set.config_file_name());

  if (p_set.p_ue4_setting.has_path() && !FSys::exists(p_set.p_ue4_setting.get_path() / staticValue::ue_path_obj())) {
    p_set.p_ue4_setting.set_path({});
    DOODLE_LOG_INFO("在路径中没有找到ue");
  }
  if (!FSys::exists(p_set.p_mayaPath / "maya.exe")) {
    DOODLE_LOG_INFO("在路径中没有找到maya");
  }
  FSys::ofstream l_ofstream{p_set.p_doc / p_set.config_file_name(), std::ios::out | std::ios::binary};
  boost::archive::text_oarchive l_out{l_ofstream};
  l_out << p_set;
  return true;
}
bool core_set_init::find_cache_dir() {
  DOODLE_LOG_INFO("寻找缓存路径");

  const static std::vector<FSys::path> dirs{"D:/",
                                            "E:/",
                                            "F:/",
                                            "G:/",
                                            "H:/",
                                            "I:/",
                                            "J:/",
                                            "K:/",
                                            "L:/"};
  auto l_item = std::any_of(dirs.begin(), dirs.end(), [this](const FSys::path &in_path) {
    try {
      if (FSys::exists(in_path)) {
        auto info = FSys::space(in_path);
        if (((float)info.available / (float)info.capacity) > 0.2) {
          p_set.set_root(in_path / "Doodle");
          return true;
        }
      }
    } catch (const FSys::filesystem_error &e) {
      DOODLE_LOG_ERROR(e.what())
    }
    return false;
  });
  return l_item;
}

bool core_set_init::config_to_user() {
  p_set.p_doc = win::get_pwd() / "doodle";
  if (!FSys::exists(p_set.p_doc)) {
    FSys::create_directories(p_set.p_doc);
  }
  return true;
}

}  // namespace doodle
