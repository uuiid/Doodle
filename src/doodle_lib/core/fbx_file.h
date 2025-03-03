//
// Created by TD on 25-2-28.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
namespace doodle::fbx {
std::vector<std::string> get_all_materials(const FSys::path& in_path, bool in_split_namespace = true);
}  // namespace doodle::fbx
