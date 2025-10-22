#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <filesystem>
#include <memory>
#include <torch/csrc/api/include/torch/torch.h>
#include <vector>

namespace doodle::ai {

class bone_weight_inference_model {
  class impl;
  std::shared_ptr<impl> pimpl_;

 public:
  bone_weight_inference_model()  = default;
  ~bone_weight_inference_model() = default;
  explicit bone_weight_inference_model(const FSys::path& in_model_path) { load_model(in_model_path); }

  void load_model(const FSys::path& in_model_path);
  FSys::path get_model_path() const;

  torch::Tensor predict(
      const torch::Tensor& node_features, const torch::Tensor& node_adj, const torch::Tensor& bone_emb = {}
  );
  // 训练模型
  static std::shared_ptr<bone_weight_inference_model> train(
      const std::vector<FSys::path>& in_fbx_files, const FSys::path& in_output_path
  );
};

}  // namespace doodle::ai