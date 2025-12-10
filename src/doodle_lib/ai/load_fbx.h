#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <filesystem>
#include <torch/csrc/api/include/torch/torch.h>
#include <vector>

namespace doodle::ai {

struct fbx_load_result {
  torch::Tensor vertices_;        // [num_vertices, 3]
  torch::Tensor bone_positions_;  // [num_bones, 3]
};
}  // namespace doodle::ai