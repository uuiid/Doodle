//
// Created by TD on 26-11-23.
//

#pragma once
#include <doodle_core/lib_warp/json_warp.h>

#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>

#include <cstdint>
#include <string>

namespace doodle::http {

enum class desktop_platform_enum { win32, darwin, linux };
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    desktop_platform_enum, {
                               {desktop_platform_enum::win32, "win32"},
                               {desktop_platform_enum::darwin, "darwin"},
                               {desktop_platform_enum::linux, "linux"},
                           }
)

enum class desktop_arch_enum { x64, arm64 };
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    desktop_arch_enum, {
                           {desktop_arch_enum::x64, "x64"},
                           {desktop_arch_enum::arm64, "arm64"},
                       }
)

enum class desktop_channel_enum { latest, beta };
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    desktop_channel_enum, {
                              {desktop_channel_enum::latest, "latest"},
                              {desktop_channel_enum::beta, "beta"},
                          }
)

struct desktop_update_latest_t {
  std::string version_{};
  std::string published_at_{};
  bool mandatory_{false};
  std::string min_supported_version_{};
  std::string notes_{};
  std::string notes_url_{};

  friend void to_json(nlohmann::json& in_json, const desktop_update_latest_t& in_latest);
  friend void from_json(const nlohmann::json& in_json, desktop_update_latest_t& in_latest);
};

struct desktop_update_installer_t {
  std::string url_{};
  std::int64_t size_{0};
  std::string sha512_{};

  friend void to_json(nlohmann::json& in_json, const desktop_update_installer_t& in_installer);
  friend void from_json(const nlohmann::json& in_json, desktop_update_installer_t& in_installer);
};

struct desktop_update_t {
  desktop_update_latest_t latest_{};
  desktop_update_installer_t installer_{};

  void check() const;

  friend void to_json(nlohmann::json& in_json, const desktop_update_t& in_update);
  friend void from_json(const nlohmann::json& in_json, desktop_update_t& in_update);
};

// /api/desktop/update/{platform}/{arch}/{channel}
DOODLE_HTTP_JWT_FUN(desktop_update)
protected:
void parse_header(const session_data_ptr& in_handle) override;

public:
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/desktop/update/{platform_}/{arch_}/{channel_}", platform_, arch_, channel_)
desktop_platform_enum platform_{};
desktop_arch_enum arch_{};
desktop_channel_enum channel_{};
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http
