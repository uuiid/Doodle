//
// Created by TD on 26-11-23.
//

#include "desktop_update.h"

#include <doodle_lib/core/global_function.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_method/kitsu.h>

#include <boost/beast/http/status.hpp>

#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

namespace doodle::http {
namespace {
nlohmann::json load_desktop_update(const std::string& in_path) {
  const auto l_empty = nlohmann::json{{"has_update", false}};
  std::error_code l_code{};
  if (!FSys::exists(in_path, l_code) || l_code) {
    SPDLOG_LOGGER_WARN(g_logger_ctrl().get_http(), "客户端升级配置不存在: {}", in_path);
    return l_empty;
  }
  std::ifstream l_file{in_path};
  if (!l_file.is_open()) {
    SPDLOG_LOGGER_WARN(g_logger_ctrl().get_http(), "客户端升级配置无法打开: {}", in_path);
    return l_empty;
  }
  auto l_json = nlohmann::json::parse(l_file, nullptr, false);
  if (l_json.is_discarded() || !l_json.is_object()) {
    SPDLOG_LOGGER_WARN(g_logger_ctrl().get_http(), "客户端升级配置不是合法的 JSON 对象: {}", in_path);
    return l_empty;
  }
  const bool l_has_installer = l_json.contains("installer") && l_json["installer"].is_object() &&
                               l_json["installer"].contains("url") && l_json["installer"]["url"].is_string() &&
                               !l_json["installer"]["url"].get<std::string>().empty() &&
                               l_json["installer"].contains("sha512") && l_json["installer"]["sha512"].is_string() &&
                               !l_json["installer"]["sha512"].get<std::string>().empty();
  const bool l_has_version =
      l_json.contains("latest") && l_json["latest"].is_object() && l_json["latest"].contains("version") &&
      l_json["latest"]["version"].is_string() && !l_json["latest"]["version"].get<std::string>().empty();
  if (!l_has_installer || !l_has_version) {
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_http(), "客户端升级配置缺少必需字段 latest.version / installer.url / installer.sha512: {}",
        in_path
    );
    return l_empty;
  }
  return l_json;
}
}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(desktop_update, get) {
  auto l_path = g_kitsu_ctx().get_desktop_update_file();
  SPDLOG_LOGGER_DEBUG(
      g_logger_ctrl().get_http(), "客户端检查更新 platform {} arch {} channel {}: {}",
      magic_enum::enum_name(platform_), magic_enum::enum_name(arch_), magic_enum::enum_name(channel_),
      l_path.generic_string()
  );
  co_return std::make_unique<http_body_json_t>(load_desktop_update(l_path.generic_string()));
}

}  // namespace doodle::http
