#include "core_set.h"

#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/lib_warp/boost_uuid_warp.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/user.h>

#include <boost/algorithm/string.hpp>
#include <boost/dll.hpp>

#include "configure/static_value.h"
#include <ShlObj.h>

namespace doodle {

FSys::path win::get_pwd() {
  /// 这里我们手动做一些工作
  /// 获取环境变量 FOLDERID_Documents
  PWSTR pManager;
  SHGetKnownFolderPath(FOLDERID_Documents, NULL, nullptr, &pManager);
  if (!pManager) throw_error(error_enum::null_string);

  auto k_path = FSys::path{pManager};
  CoTaskMemFree(pManager);
  return k_path;
}

FSys::path win::get_font() {
  /// 这里我们手动做一些工作
  /// 获取环境变量 FOLDERID_Documents
  PWSTR pManager;
  SHGetKnownFolderPath(FOLDERID_Fonts, NULL, nullptr, &pManager);
  if (!pManager) throw_error(error_enum::null_string);

  auto k_path = FSys::path{pManager};
  CoTaskMemFree(pManager);
  return k_path;
}

core_set &core_set::get_set() {
  static core_set install{};
  return install;
}

core_set::core_set()
    : user_id(),
      p_doc(FSys::current_path()),
      p_max_thread(std::thread::hardware_concurrency() - 2),
      p_root(FSys::temp_directory_path() / "Doodle"),
      _root_cache(p_root / "cache"),
      timeout(3600),
      maya_version(2019),
      assets_file_widgets_size(5),
      json_data(std::make_shared<nlohmann::json>()) {
  auto l_short_path = FSys::temp_directory_path().generic_wstring();
  auto k_buff_size  = GetLongPathNameW(l_short_path.c_str(), nullptr, 0);
  std::unique_ptr<wchar_t[]> const p_buff{new wchar_t[k_buff_size]};
  auto l_r = GetLongPathNameW(l_short_path.c_str(), p_buff.get(), k_buff_size);
  if (FAILED(l_r)) {
    set_root("C:/");
  } else {
    set_root(FSys::path{p_buff.get()} / "Doodle");
  }

  if (boost::dll::program_location().filename() == "DoodleExe.exe") {
    program_location_attr = boost::dll::program_location().generic_string();
  }
  user_id = get_uuid();
}

boost::uuids::uuid core_set::get_uuid() { return p_uuid_gen(); }

FSys::path core_set::get_doc() const { return p_doc; }

void core_set::set_root(const FSys::path &in_root) {
  p_root      = in_root;
  _root_cache = p_root / "cache";
}

FSys::path core_set::get_cache_root() const { return _root_cache; }

FSys::path core_set::get_cache_root(const FSys::path &in_path) const {
  auto path = get_cache_root() / in_path;
  if (!FSys::exists(path)) FSys::create_directories(path);
  return path;
}

FSys::path core_set::program_location() { return program_location_attr.parent_path(); }

std::string core_set::get_uuid_str() { return boost::uuids::to_string(get_uuid()); }

/// ----------------------------------------------------------------------------
/// ----------------------------------------------------------------------------
/// ----------------------------------------------------------------------------

core_set_init::core_set_init() : p_set(core_set::get_set()) {
  if (!FSys::exists(p_set.p_doc)) FSys::create_directories(p_set.p_doc);
  if (!FSys::exists(p_set.get_cache_root())) {
    FSys::create_directories(p_set.get_cache_root());
  }

  DOODLE_LOG_INFO("设置缓存目录 {}", p_set.p_root);
}

void core_set_init::read_file() {
  FSys::path l_k_setting_file_name = p_set.get_doc() / doodle_config::config_name;
  DOODLE_LOG_INFO("读取配置文件 {}", l_k_setting_file_name);
  if (FSys::exists(l_k_setting_file_name)) {
    FSys::ifstream l_in_josn{l_k_setting_file_name, std::ifstream::binary};
    try {
      *p_set.json_data = nlohmann::json::parse(l_in_josn);
      p_set.json_data->at("setting").get_to(p_set);

    } catch (const nlohmann::json::parse_error &err) {
      DOODLE_LOG_DEBUG(boost::diagnostic_information(err));
    }
  }
  p_set.json_data = std::make_shared<nlohmann::json>();
  if (p_set.user_id.is_nil()) {
    p_set.user_id = p_set.get_uuid();
  }
  g_reg()->ctx().get<user::current_user>().uuid = p_set.user_id;
}
bool core_set_init::write_file() {
  DOODLE_LOG_INFO("写入配置文件 {}", p_set.p_doc / doodle_config::config_name);

  FSys::ofstream l_ofstream{p_set.p_doc / doodle_config::config_name, std::ios::out | std::ios::binary};
  (*p_set.json_data)["setting"] = p_set;

  l_ofstream << p_set.json_data->dump();
  return true;
}

bool core_set_init::config_to_user() {
  p_set.p_doc = win::get_pwd() / "doodle";
  if (!FSys::exists(p_set.p_doc)) {
    FSys::create_directories(p_set.p_doc);
  }
  return true;
}

void to_json(nlohmann::json &j, const core_set &p) {
  j["organization_name"]        = p.organization_name;
  j["max_thread"]               = p.p_max_thread;
  j["timeout"]                  = p.timeout;
  j["project_root"]             = p.project_root;
  j["ue4_path"]                 = p.ue4_path;
  j["ue4_version"]              = p.ue4_version;
  j["maya_replace_save_dialog"] = p.maya_replace_save_dialog;
  j["maya_force_resolve_link"]  = p.maya_force_resolve_link;
  j["user_id"]                  = p.user_id;
  j["user_name"]                = p.user_name;
  j["program_location_attr"]    = p.program_location_attr;
  j["server_ip"]                = p.server_ip;
  j["maya_version"]             = p.maya_version;
  j["layout_config"]            = p.layout_config;
  j["assets_file_widgets_size"] = p.assets_file_widgets_size;
}

void from_json(const nlohmann::json &j, core_set &p) {
  if (j.count("organization_name")) j.at("organization_name").get_to(p.organization_name);
  if (j.count("ue4_setting")) {
    j["ue4_setting"].at("ue4_path").get_to(p.ue4_path);
    j["ue4_setting"].at("ue4_version").get_to(p.ue4_version);
  }
  if (j.count("ue4_path")) j["ue4_path"].get_to(p.ue4_path);
  if (j.count("ue4_version")) j["ue4_version"].get_to(p.ue4_version);
  j.at("max_thread").get_to(p.p_max_thread);
  j.at("timeout").get_to(p.timeout);
  if (j.contains("project_root")) j.at("project_root").get_to(p.project_root);
  if (j.contains("maya_replace_save_dialog")) j.at("maya_replace_save_dialog").get_to(p.maya_replace_save_dialog);
  if (j.contains("maya_force_resolve_link")) j.at("maya_force_resolve_link").get_to(p.maya_force_resolve_link);
  if (j.contains("user_id"))
    j.at("user_id").get_to(p.user_id);
  else
    p.user_id = p.get_uuid();
  /// \brief 兼容旧版本段配置文件
  if (j.contains("user_")) j.at("user_").get_to(p.user_name);
  if (j.contains("user_name")) j.at("user_name").get_to(p.user_name);

  if (j.contains("program_location_attr") && p.program_location_attr.empty()) {
    p.program_location_attr = j.at("program_location_attr").get<FSys::path>();
  }
  if (j.contains("server_ip")) j.at("server_ip").get_to(p.server_ip);
  if (j.contains("maya_version")) j.at("maya_version").get_to(p.maya_version);
  if (j.contains("layout_config")) j.at("layout_config").get_to(p.layout_config);
  if (j.contains("assets_file_widgets_size")) j.at("assets_file_widgets_size").get_to(p.assets_file_widgets_size);
}
void core_set::add_recent_project(const FSys::path &in) {
  auto k_find_root = std::find(project_root.begin(), project_root.end(), in);
  if (k_find_root != project_root.end())
    std::swap(project_root[0], *k_find_root);
  else {
    std::rotate(project_root.rbegin(), project_root.rbegin() + 1, project_root.rend());
    project_root[0] = in;
  }
}
std::string core_set::get_uuid_str(const std::string &in_add) { return get_uuid_str() + in_add; }

}  // namespace doodle
