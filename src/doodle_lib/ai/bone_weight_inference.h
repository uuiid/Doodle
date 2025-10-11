#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <filesystem>
#include <vector>

namespace doodle::ai {

/**
 * @brief 骨骼权重推断, 从fbx文件推断出骨骼权重的ai模型
 */
FSys::path run_bone_weight_inference(const std::vector<FSys::path>& in_fbx_files, const FSys::path& in_output_path);
}  // namespace doodle::ai