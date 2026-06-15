//
// Created by teXiao on 2021/4/27.
//

#include "metadata.h"

#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/project_status.h>

namespace doodle {
constexpr uuid project_status::get_open_id() {
  return uuid{{0x75, 0x5c, 0x9e, 0xdd, 0x94, 0x81, 0x41, 0x45, 0xab, 0x43, 0x21, 0x49, 0x1b, 0xdf, 0x27, 0x39}};
}
constexpr uuid project_status::get_closed_id() {
  return uuid{{0x51, 0x59, 0xf2, 0x10, 0x7e, 0xc8, 0x40, 0xe3, 0xb8, 0xc9, 0x2a, 0x06, 0xd0, 0xb4, 0xb1, 0x16}};
}
constexpr std::array<project_status, 2> project_status::get_all_constant() {
  return {
      project_status{.uuid_id_ = get_open_id(), .name_ = "Open", .color_ = "#000000"},
      project_status{.uuid_id_ = get_closed_id(), .name_ = "Closed", .color_ = "#000000"}
  };
}
constexpr uuid assets_helper::database_t::get_lable_id() {
  return uuid{{0x01, 0x96, 0xeb, 0x9d, 0x5d, 0xc0, 0x72, 0x7d, 0x8a, 0x75, 0x1b, 0x05, 0xde, 0xa8, 0x49, 0x4d}};
}
constexpr std::array<assets_helper::database_t, 1> assets_helper::database_t::get_all_constant() {
  return {assets_helper::database_t{.uuid_id_ = get_lable_id(), .label_ = "标签", .uuid_parent_ = {}, .order_ = 0}};
}

}  // namespace doodle
