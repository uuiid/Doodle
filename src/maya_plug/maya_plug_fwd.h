#pragma once

#include <maya_plug/data/reference_file.h>

#include <memory>
namespace doodle::maya_plug {

namespace reference_file_ns {
class generate_fbx_file_path;
}

using generate_file_path_ptr = std::shared_ptr<reference_file_ns::generate_file_path_base>;
class file_info_edit;
}  // namespace doodle::maya_plug