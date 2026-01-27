#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <fbxsdk.h>
#include <filesystem>
#include <torch/csrc/api/include/torch/data.h>
#include <torch/csrc/api/include/torch/torch.h>
#include <vector>


namespace doodle::ai {

struct fbx_load_result {
  torch::Tensor vertices_;        // [num_vertices, 3]
  torch::Tensor normals_;         // [num_vertices, 3]
  torch::Tensor faces_;           // [num_faces, 3]
  torch::Tensor neighbor_idx_;    // [N, k] int64
  torch::Tensor topo_degree_;     // [N] float32
  torch::Tensor curvature_;       // [num_vertices]
  torch::Tensor bone_positions_;  // [num_bones, 3]
  torch::Tensor bone_weights_;    // [num_vertices, num_bones]
  torch::Tensor bone_parents_;    // [num_bones]
  torch::Tensor bones_dir_len_;   // [num_bones, 3]
  // 骨骼到点的距离
  torch::Tensor bone_to_point_dist_;  // [num_vertices, num_bones]

  // topo_degree is scalar connectivity feature per-vertex, derived from faces_
  void build_face_adjacency(std::int64_t k);
  // 归一化
  void normalize_inputs();
  // 计算骨骼方向
  void compute_bones_dir_len();

  // 计算 curvature
  void compute_curvature();
  // 计算骨骼到点的距离
  void compute_bone_to_point_dist();

  void save(const FSys::path& out_path) const;
  void load(const FSys::path& in_path);

  template <typename T>
  inline void to(T in_opt) {
    vertices_           = vertices_.to(in_opt);
    normals_            = normals_.to(in_opt);
    faces_              = faces_.to(in_opt);
    neighbor_idx_       = neighbor_idx_.to(in_opt);
    topo_degree_        = topo_degree_.to(in_opt);
    curvature_          = curvature_.to(in_opt);
    bone_positions_     = bone_positions_.to(in_opt);
    bone_weights_       = bone_weights_.to(in_opt);
    bone_parents_       = bone_parents_.to(in_opt);
    bones_dir_len_      = bones_dir_len_.to(in_opt);
    bone_to_point_dist_ = bone_to_point_dist_.to(in_opt);
  }
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