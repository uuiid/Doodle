//
// Created by TD on 2022/4/12.
//

#pragma once

#include <doodle_core/configure/static_value.h>

#include <string>
#include <array>

namespace doodle {
namespace gui::config::maya_plug::menu {
constexpr static std::string_view comm_check_scenes{"检查工具"};
constexpr static std::string_view create_sim_cloth{"创建布料"};
constexpr static std::string_view reference_attr_setting{"引用编辑"};
constexpr static std::string_view dem_cloth_to_fbx{"布料转换"};
constexpr static auto menu_maya =
    std::tuple_cat(gui::config::menu_w::menu_base, std::make_tuple(comm_check_scenes, create_sim_cloth, reference_attr_setting));

}  // namespace gui::config::maya_plug::menu
}  // namespace doodle
