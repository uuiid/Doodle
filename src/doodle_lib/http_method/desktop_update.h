//
// Created by TD on 26-11-23.
//

#pragma once
#include <doodle_core/lib_warp/json_warp.h>

#include <doodle_lib/core/http/http_route.h>

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

// /api/desktop/update/{platform}/{arch}/{channel}
DOODLE_HTTP_FUN(desktop_update)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/desktop/update/{platform_}/{arch_}/{channel_}", platform_, arch_, channel_)
desktop_platform_enum platform_{};
desktop_arch_enum arch_{};
desktop_channel_enum channel_{};
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http
