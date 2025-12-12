#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <fbxsdk.h>
#include <filesystem>
#include <torch/csrc/api/include/torch/torch.h>
#include <vector>

namespace doodle::ai {

struct fbx_load_result {
  torch::Tensor vertices_;                  // [num_vertices, 3]
  torch::Tensor normals_;                   // [num_vertices, 3]
  torch::Tensor faces_;                     // [num_faces, 3]
  torch::Tensor curvature_;                 // [num_vertices]
  torch::Tensor degree_;                    // [num_vertices]
  torch::Tensor normal_deviation_;          // [num_vertices]
  torch::Tensor bone_positions_;            // [num_bones, 3]
  torch::Tensor bone_weights_;              // [num_vertices, num_bones]
  torch::Tensor bone_parents_;  // [num_bones]
  torch::Tensor bones_dir_len_;             // [num_bones, 4] optional
};

class fbx_loader {
  std::shared_ptr<fbxsdk::FbxManager> manager_;
  fbxsdk::FbxScene* scene_;
  FSys::path fbx_path_;
  FbxNode* mesh_node_{};
  fbxsdk::FbxMesh* mesh_{};
  fbxsdk::FbxVector4 root_offset_{};

  logger_ptr_raw logger_;
  void preprocessing();

 public:
  explicit fbx_loader(const FSys::path& in_fbx_path, logger_ptr_raw in_logger = nullptr);
  fbx_load_result load_fbx();
  void write_weights_to_fbx(const torch::Tensor& weights, const std::filesystem::path& out_path);

 private:
  void write_fbx(const FSys::path& out_path);
};

}  // namespace doodle::ai