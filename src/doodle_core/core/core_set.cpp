#include "core_set.h"

#include <doodle_core/configure/static_value.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/lib_warp/boost_uuid_warp.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/user.h>

#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>

#include <ShlObj.h>
#include <wil/result.h>

namespace doodle {

FSys::path win::get_pwd() {
  /// 这里我们手动做一些工作
  /// 获取环境变量 FOLDERID_Documents
  wil::unique_cotaskmem_string pManager_ptr{};
  LOG_IF_FAILED(::SHGetKnownFolderPath(FOLDERID_Documents, NULL, nullptr, pManager_ptr.put()));
  if (!pManager_ptr) return {};
  auto k_path = FSys::path{pManager_ptr.get()};
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
      p_doc(win::get_pwd()),
      p_max_thread(std::thread::hardware_concurrency() - 2),
      p_root(FSys::temp_directory_path() / "Doodle"),
      _root_cache(p_root / "cache"),
      timeout(3600),
      maya_version(2020),
      assets_file_widgets_size(5),
      p_uuid_gen(boost::uuids::random_generator{}),
      server_ip("http://192.168.40.181:50026"),
#ifdef NDEBUG
      depot_ip{"\\\\192.168.10.218\\Doodletemp"}
#else
      depot_ip{"\\\\192.168.20.89\\UE_Config\\Doodletemp"}
#endif
{
  p_doc /= "doodle";
  if (!FSys::exists(p_doc)) FSys::create_directories(p_doc);

  auto l_short_path = FSys::temp_directory_path().generic_wstring();
  auto k_buff_size  = GetLongPathNameW(l_short_path.c_str(), nullptr, 0);
  std::unique_ptr<wchar_t[]> const p_buff{new wchar_t[k_buff_size]};
  auto l_r = GetLongPathNameW(l_short_path.c_str(), p_buff.get(), k_buff_size);
  if (FAILED(l_r)) {
    set_root("C:/");
  } else {
    set_root(FSys::path{p_buff.get()} / "Doodle");
  }

  if (!FSys::exists(get_cache_root())) {
    FSys::create_directories(get_cache_root());
  }
  utf8_locale = boost::locale::generator().generate("zh_CN.UTF-8");

  if (FSys::path l_k_setting_file_name = get_doc() / doodle_config::config_name; FSys::exists(l_k_setting_file_name)) {
    default_logger_raw()->log(log_loc(), level::warn, "读取配置文件 {}", l_k_setting_file_name);
    try {
      FSys::ifstream l_in_josn{l_k_setting_file_name, std::ifstream::binary};
      auto l_data = nlohmann::json::parse(l_in_josn);
      l_data.at("setting").get_to(*this);
    } catch (const nlohmann::json::parse_error &err) {
      DOODLE_LOG_DEBUG(boost::diagnostic_information(err));
    }
  }
  if (user_id.is_nil()) {
    user_id = get_uuid();
  }
}

boost::uuids::uuid core_set::get_uuid() { return p_uuid_gen(); }

FSys::path core_set::get_doc() const { return p_doc; }

std::string core_set::get_render_url() {
#ifdef NDEBUG
  return {"http://192.168.40.181:50023"};
#else
  return {"http://192.168.20.89:50023"};
#endif
}

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

std::string core_set::get_uuid_str() { return boost::uuids::to_string(get_uuid()); }

void core_set::save() {
  try {
    auto l_k_setting_file_name = get_doc() / doodle_config::config_name;
    FSys::ofstream l_out_josn{l_k_setting_file_name, std::ifstream::binary};
    l_out_josn << nlohmann::json{{"setting", *this}}.dump();
  } catch (...) {
    default_logger_raw()->error("保存配置文件错误 {}", boost::current_exception_diagnostic_information());
  }
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
  j["maya_version"]             = p.maya_version;
  j["layout_config"]            = p.layout_config;
  j["assets_file_widgets_size"] = p.assets_file_widgets_size;
  j["depot_ip"]                 = p.depot_ip;
  j["next_time_"]               = p.next_time_;
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

  if (j.contains("maya_version")) j.at("maya_version").get_to(p.maya_version);
  if (j.contains("layout_config")) j.at("layout_config").get_to(p.layout_config);
  if (j.contains("assets_file_widgets_size")) j.at("assets_file_widgets_size").get_to(p.assets_file_widgets_size);
  if (j.contains("depot_ip")) j.at("depot_ip").get_to(p.depot_ip);
  if (j.contains("next_time_")) j.at("next_time_").get_to(p.next_time_);
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
