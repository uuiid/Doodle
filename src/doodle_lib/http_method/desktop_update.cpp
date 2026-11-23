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
struct desktop_update_latest_t {
  std::string version_{};
  std::string published_at_{};
  bool mandatory_{false};
  std::string min_supported_version_{};
  std::string notes_{};
  std::string notes_url_{};

  friend void to_json(nlohmann::json& in_json, const desktop_update_latest_t& in_latest) {
    in_json["version"] = in_latest.version_;
    if (!in_latest.published_at_.empty()) in_json["published_at"] = in_latest.published_at_;
    in_json["mandatory"] = in_latest.mandatory_;
    if (!in_latest.min_supported_version_.empty())
      in_json["min_supported_version"] = in_latest.min_supported_version_;
    if (!in_latest.notes_.empty()) in_json["notes"] = in_latest.notes_;
    if (!in_latest.notes_url_.empty()) in_json["notes_url"] = in_latest.notes_url_;
  }
  friend void from_json(const nlohmann::json& in_json, desktop_update_latest_t& in_latest) {
    in_json.at("version").get_to(in_latest.version_);
    if (in_json.contains("published_at")) in_json["published_at"].get_to(in_latest.published_at_);
    if (in_json.contains("mandatory")) in_json["mandatory"].get_to(in_latest.mandatory_);
    if (in_json.contains("min_supported_version"))
      in_json["min_supported_version"].get_to(in_latest.min_supported_version_);
    if (in_json.contains("notes")) in_json["notes"].get_to(in_latest.notes_);
    if (in_json.contains("notes_url")) in_json["notes_url"].get_to(in_latest.notes_url_);
  }
};

struct desktop_update_installer_t {
  std::string url_{};
  std::int64_t size_{0};
  std::string sha512_{};

  friend void to_json(nlohmann::json& in_json, const desktop_update_installer_t& in_installer) {
    in_json["url"]    = in_installer.url_;
    in_json["size"]   = in_installer.size_;
    in_json["sha512"] = in_installer.sha512_;
  }
  friend void from_json(const nlohmann::json& in_json, desktop_update_installer_t& in_installer) {
    in_json.at("url").get_to(in_installer.url_);
    in_json.at("sha512").get_to(in_installer.sha512_);
    if (in_json.contains("size")) in_json["size"].get_to(in_installer.size_);
  }
};

struct desktop_update_t {
  desktop_update_latest_t latest_{};
  desktop_update_installer_t installer_{};

  void check() const {
    DOODLE_CHICK(!latest_.version_.empty(), "latest.version 为空");
    DOODLE_CHICK(!installer_.url_.empty(), "installer.url 为空");
    DOODLE_CHICK(!installer_.sha512_.empty(), "installer.sha512 为空");
  }

  friend void to_json(nlohmann::json& in_json, const desktop_update_t& in_update) {
    in_json["latest"]    = in_update.latest_;
    in_json["installer"] = in_update.installer_;
  }
  friend void from_json(const nlohmann::json& in_json, desktop_update_t& in_update) {
    in_json.at("latest").get_to(in_update.latest_);
    in_json.at("installer").get_to(in_update.installer_);
    in_update.check();
  }
};

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
  if (l_json.is_discarded()) {
    SPDLOG_LOGGER_WARN(g_logger_ctrl().get_http(), "客户端升级配置不是合法的 JSON: {}", in_path);
    return l_empty;
  }
  try {
    nlohmann::json l_out{};
    l_out = l_json.get<desktop_update_t>();
    return l_out;
  } catch (const std::exception& in_error) {
    SPDLOG_LOGGER_WARN(g_logger_ctrl().get_http(), "客户端升级配置无效 {}: {}", in_path, in_error.what());
    return l_empty;
  }
}
}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(desktop_update, get) {
  const std::string l_platform{magic_enum::enum_name(platform_)};
  const std::string l_arch{magic_enum::enum_name(arch_)};
  const std::string l_channel{magic_enum::enum_name(channel_)};
  auto l_path = g_kitsu_ctx().get_desktop_update_file(l_platform, l_arch, l_channel);
  SPDLOG_LOGGER_DEBUG(
      g_logger_ctrl().get_http(), "客户端检查更新 platform {} arch {} channel {}: {}", l_platform, l_arch, l_channel,
      l_path.generic_string()
  );
  co_return std::make_unique<http_body_json_t>(load_desktop_update(l_path.generic_string()));
}

}  // namespace doodle::http
