#include "cross_attention_bone_weight.h"

#include "doodle_core/exception/exception.h"

#include <doodle_lib/ai/load_fbx.h>

#include <ATen/core/TensorBody.h>
#include <algorithm>
#include <c10/util/Load.h>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>
#include <torch/torch.h>
#include <torch/types.h>
#include <vector>


namespace doodle::ai {
void fbx_load_result::compute_curvature() {
  // Simple curvature estimation using vertex normals and neighboring vertices
  DOODLE_CHICK(vertices_.defined(), "vertices tensor is undefined");
  DOODLE_CHICK(normals_.defined(), "normals tensor is undefined");
  DOODLE_CHICK(faces_.defined(), "faces tensor is undefined");

  auto num_verts = vertices_.size(0);
  curvature_     = torch::zeros({num_verts}, torch::kFloat32);

  // Build face adjacency if not already built
  if (!neighbor_idx_.defined() || !topo_degree_.defined()) {
    build_face_adjacency(5);  // Using k=5 neighbors for curvature estimation
  }

  auto vert_acc   = vertices_.accessor<float, 2>();
  auto norm_acc   = normals_.accessor<float, 2>();
  auto neigh_acc  = neighbor_idx_.accessor<int64_t, 2>();
  auto curv_acc   = curvature_.accessor<float, 1>();

  const int64_t K = neighbor_idx_.size(1);

  for (int64_t i = 0; i < num_verts; ++i) {
    float curv_sum    = 0.0f;
    const int64_t deg = static_cast<int64_t>(topo_degree_[i].item<float>());
    const int64_t k   = std::min<int64_t>(deg, K);
    for (int64_t j = 0; j < k; ++j) {
      int64_t nbr_idx   = neigh_acc[i][j];
      // Compute angle between normals
      float dot_product = norm_acc[i][0] * norm_acc[nbr_idx][0] + norm_acc[i][1] * norm_acc[nbr_idx][1] +
                          norm_acc[i][2] * norm_acc[nbr_idx][2];
      dot_product = std::clamp(dot_product, -1.0f, 1.0f);
      float angle = std::acos(dot_product);
      curv_sum += angle;
    }
    if (k > 0) {
      curv_acc[i] = curv_sum / static_cast<float>(k);
    } else {
      curv_acc[i] = 0.0f;
    }
  }
}
void fbx_load_result::build_face_adjacency(std::int64_t k) {
  auto l_num_verts = vertices_.size(0);
  DOODLE_CHICK(faces_.defined(), "faces tensor is undefined");
  DOODLE_CHICK(faces_.dim() == 2, "faces must be 2D [F,3]");
  DOODLE_CHICK(faces_.size(1) >= 3, "faces must have at least 3 columns");
  DOODLE_CHICK(l_num_verts > 0, "num_verts must be > 0");
  DOODLE_CHICK(k > 0, "k must be > 0");

  // Build adjacency on CPU for simplicity/stability.
  auto faces_cpu = faces_.to(torch::kCPU).to(torch::kInt64).contiguous();

  std::vector<std::vector<int64_t>> adj(static_cast<size_t>(l_num_verts));
  auto acc        = faces_cpu.accessor<int64_t, 2>();
  const int64_t F = faces_cpu.size(0);
  for (int64_t f = 0; f < F; ++f) {
    const int64_t a = acc[f][0];
    const int64_t b = acc[f][1];
    const int64_t c = acc[f][2];
    if (a < 0 || b < 0 || c < 0) {
      continue;
    }
    if (a >= l_num_verts || b >= l_num_verts || c >= l_num_verts) {
      continue;
    }
    // undirected edges
    adj[static_cast<size_t>(a)].push_back(b);
    adj[static_cast<size_t>(a)].push_back(c);
    adj[static_cast<size_t>(b)].push_back(a);
    adj[static_cast<size_t>(b)].push_back(c);
    adj[static_cast<size_t>(c)].push_back(a);
    adj[static_cast<size_t>(c)].push_back(b);
  }

  auto idx_cpu = torch::empty({l_num_verts, k}, torch::TensorOptions().dtype(torch::kInt64).device(torch::kCPU));
  auto deg_cpu = torch::empty({l_num_verts}, torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU));
  auto idx_acc = idx_cpu.accessor<int64_t, 2>();
  auto deg_acc = deg_cpu.accessor<float, 1>();

  for (int64_t v = 0; v < l_num_verts; ++v) {
    auto& n = adj[static_cast<size_t>(v)];
    if (!n.empty()) {
      std::sort(n.begin(), n.end());
      n.erase(std::unique(n.begin(), n.end()), n.end());
    }

    deg_acc[v] = static_cast<float>(n.size());

    // Fill fixed-size neighbor list. If not enough neighbors, pad with self.
    for (int64_t j = 0; j < k; ++j) {
      if (j < static_cast<int64_t>(n.size())) {
        idx_acc[v][j] = n[static_cast<size_t>(j)];
      } else {
        idx_acc[v][j] = v;
      }
    }
  }
  neighbor_idx_ = idx_cpu;
  topo_degree_  = deg_cpu;
}

void fbx_load_result::normalize_inputs() {
  // Normalize inputs to avoid large values causing model collapse
  auto max_val = vertices_.abs().max().item<float>();
  if (max_val < 1e-6) max_val = 1.0;
  vertices_       = vertices_ / max_val;
  bone_positions_ = bone_positions_ / max_val;
  // 归中
  auto centroid   = vertices_.mean(0, true);
  vertices_       = vertices_ - centroid;
  bone_positions_ = bone_positions_ - centroid;
}
void fbx_load_result::compute_bones_dir_len() {
  auto num_bones    = bone_positions_.size(0);
  bones_dir_len_    = torch::zeros({num_bones, 3}, torch::kFloat32);
  auto bone_pos_acc = bone_positions_.accessor<float, 2>();
  auto bone_dir_acc = bones_dir_len_.accessor<float, 2>();
  for (int64_t i = 0; i < num_bones; ++i) {
    int64_t parent_idx = bone_parents_[i].item<int64_t>();
    if (parent_idx >= 0 && parent_idx < num_bones) {
      float dir_x        = bone_pos_acc[i][0] - bone_pos_acc[parent_idx][0];
      float dir_y        = bone_pos_acc[i][1] - bone_pos_acc[parent_idx][1];
      float dir_z        = bone_pos_acc[i][2] - bone_pos_acc[parent_idx][2];
      bone_dir_acc[i][0] = dir_x;
      bone_dir_acc[i][1] = dir_y;
      bone_dir_acc[i][2] = dir_z;
    } else {
      // Root bone, set direction to zero
      bone_dir_acc[i][0] = 0.0f;
      bone_dir_acc[i][1] = 0.0f;
      bone_dir_acc[i][2] = 0.0f;
    }
  }
}

// ----------------------------- Utility functions --------------------------------

// Simple function to gather neighbor features: given feat [N, C], idx [N, k] -> [N, k, C]
torch::Tensor gather_neighbors(const torch::Tensor& feat, const torch::Tensor& idx) {
  // idx: LongTensor [N, k]
  auto idx_flat = idx.reshape(-1);
  auto gathered = feat.index_select(0, idx_flat);
  auto N        = idx.size(0);
  auto k        = idx.size(1);
  auto C        = feat.size(1);
  return gathered.view({N, k, C});
}

// ----------------------------- Model components --------------------------------

// EdgeConv block (simple): for each vertex i, for neighbors j in N(i), compute h_i = max_j ReLU( W * [x_i || x_j - x_i]
// )
struct EdgeConvImpl : torch::nn::Module {
  torch::nn::Linear mlp{nullptr};
  int k;
  EdgeConvImpl(int in_channels, int out_channels, int k_) : k(k_) {
    mlp = register_module("mlp", torch::nn::Linear(in_channels * 2, out_channels));
  }

  // x: [N, C] ; neighbor_idx: [N, k]
  torch::Tensor forward(const torch::Tensor& x, const torch::Tensor& neighbor_idx) {
    auto neigh    = gather_neighbors(x, neighbor_idx);  // [N,k,C]
    auto K        = neighbor_idx.size(1);
    auto xi       = x.unsqueeze(1).expand({-1, K, -1});  // [N,k,C]
    auto feat     = torch::cat({xi, neigh - xi}, -1);    // [N,k,2C]
    // linear expects [..., in_features]; apply by reshaping
    auto N        = feat.size(0);
    auto in       = feat.view({N * K, feat.size(2)});
    auto lin      = mlp->forward(in);
    auto lin_view = lin.view({N, K, -1});
    auto pooled   = std::get<0>(lin_view.max(1));  // [N, out_channels]
    return pooled;
  }
};
TORCH_MODULE(EdgeConv);

// Simple Tree-GNN layer: propagate messages parent->child and child->parent (two directions)
struct TreeGNNLayerImpl : torch::nn::Module {
  torch::nn::Linear msg_mlp{nullptr};
  TreeGNNLayerImpl(int in_features, int out_features) {
    msg_mlp = register_module("msg_mlp", torch::nn::Linear(in_features * 2, out_features));
  }

  // bone_feats: [B, C]; parent_idx: [B] (-1 for root)
  torch::Tensor forward(const torch::Tensor& bone_feats, const torch::Tensor& parent_idx) {
    int B       = bone_feats.size(0);
    int C       = bone_feats.size(1);
    auto device = bone_feats.device();

    auto out    = bone_feats.clone();
    // child->parent aggregation
    for (int i = 0; i < B; ++i) {
      int p = parent_idx[i].item<int>();
      if (p >= 0) {
        auto child_feat  = bone_feats[i];
        auto parent_feat = bone_feats[p];
        auto msg         = torch::relu(msg_mlp->forward(torch::cat({child_feat, parent_feat})));
        out[p] += msg;
      }
    }
    // parent->child propagation
    for (int i = 0; i < B; ++i) {
      int p = parent_idx[i].item<int>();
      if (p >= 0) {
        auto child_feat  = bone_feats[i];
        auto parent_feat = bone_feats[p];
        auto msg         = torch::relu(msg_mlp->forward(torch::cat({parent_feat, child_feat})));
        out[i] += msg;
      }
    }
    // optional normalization
    return torch::relu(out);
  }
};
TORCH_MODULE(TreeGNNLayer);

// MeshEncoder: stack EdgeConv layers, optional GAT omitted for brevity, then TransformerEncoder
struct MeshEncoderImpl : torch::nn::Module {
  std::vector<EdgeConv> edge_layers;
  torch::nn::Linear proj{nullptr};
  torch::nn::TransformerEncoder transformer{nullptr};
  int k;

  MeshEncoderImpl(
      int in_ch, const std::vector<int>& edge_out_channels, int trans_dim, int nhead = 8, int num_layers = 2, int k = 16
  ) {
    this->k  = k;
    int prev = in_ch;
    for (size_t i = 0; i < edge_out_channels.size(); ++i) {
      EdgeConv ec(prev, edge_out_channels[i], k);
      register_module("edgeconv" + std::to_string(i), ec);
      edge_layers.push_back(ec);
      prev = edge_out_channels[i];
    }
    proj = register_module("proj", torch::nn::Linear(prev, trans_dim));
    auto encoder_layer =
        torch::nn::TransformerEncoderLayer(torch::nn::TransformerEncoderLayerOptions(trans_dim, nhead).dropout(0.1));
    transformer = register_module("transformer", torch::nn::TransformerEncoder(encoder_layer, num_layers));
  }

  // verts: [N,3], normals: [N,3], faces: [F,3], curvature: [N]
  torch::Tensor forward(const fbx_load_result& in_fbx_data) {
    // build initial feature
    auto feat = torch::cat(
        {in_fbx_data.vertices_, in_fbx_data.normals_, in_fbx_data.curvature_.unsqueeze(1),
         in_fbx_data.topo_degree_.unsqueeze(1)},
        -1
    );  // [N, F]
    auto x = feat;
    for (size_t i = 0; i < edge_layers.size(); ++i) {
      x = edge_layers[i]->forward(x, in_fbx_data.neighbor_idx_);
    }
    // project to transformer dim
    auto x_proj    = proj->forward(x);  // [N, trans_dim]
    // transformer expects [S, N, E] where S = seq_len; we'll treat S=N, batch=1
    auto src       = x_proj.unsqueeze(1);        // [N,1,E]
    auto trans_out = transformer->forward(src);  // [N,1,E]
    auto out       = trans_out.squeeze(1);       // [N, E]
    return out;
  }
};
TORCH_MODULE(MeshEncoder);

// SkeletonEncoder: embed joints, run TreeGNN layers, output bone features
struct SkeletonEncoderImpl : torch::nn::Module {
  torch::nn::Linear embed{nullptr};
  std::vector<TreeGNNLayer> layers;
  SkeletonEncoderImpl(int in_dim, int embed_dim, int num_layers = 2) {
    embed = register_module("embed", torch::nn::Linear(in_dim, embed_dim));
    for (int i = 0; i < num_layers; i++) {
      auto l = TreeGNNLayer(embed_dim, embed_dim);
      register_module("tgnn" + std::to_string(i), l);
      layers.push_back(l);
    }
  }

  // bones_pos: [B,3], parent_idx: [B], bones_dir_len optional
  torch::Tensor forward(const fbx_load_result& in_fbx_data) {
    // initial features: pos + (dir,len)
    torch::Tensor feat = torch::cat({in_fbx_data.bone_positions_, in_fbx_data.bones_dir_len_}, -1);
    auto h             = torch::relu(embed->forward(feat));
    for (auto& l : layers) {
      h = l->forward(h, in_fbx_data.bone_parents_);
    }
    return h;  // [B, embed_dim]
  }
};
TORCH_MODULE(SkeletonEncoder);

// Cross-attention: vertex features attend to bone features -> produce weights matrix [N,B]
struct CrossAttentionImpl : torch::nn::Module {
  torch::nn::MultiheadAttention mha{nullptr};
  torch::nn::Linear head_proj{nullptr};
  int embed_dim;
  CrossAttentionImpl(int embed_dim_, int nhead = 8) : embed_dim(embed_dim_) {
    mha = register_module("mha", torch::nn::MultiheadAttention(torch::nn::MultiheadAttentionOptions(embed_dim, nhead)));
    head_proj = register_module("head_proj", torch::nn::Linear(embed_dim, embed_dim));
  }

  // vertex_feats: [N, E]; bone_feats: [B, E]
  torch::Tensor forward(const torch::Tensor& vertex_feats, const torch::Tensor& bone_feats) {
    // convert to seq_len x batch x embed
    auto q        = vertex_feats.unsqueeze(1);  // [N,1,E]
    auto k        = bone_feats.unsqueeze(1);    // [B,1,E]
    auto v        = bone_feats.unsqueeze(1);    // [B,1,E]
    // MultiheadAttention expects [L, N, E] where N=batch
    auto q2       = q;
    auto k2       = k;
    auto v2       = v;
    auto attn_out = mha->forward(q2, k2, v2);
    // attn_out is tuple (output, attn_weights)
    auto output   = std::get<0>(attn_out);  // [N,1,E]
    // Use MHA output to compute logits (scheme 1): fuse vertex features with bone context first.
    auto fused_v  = head_proj->forward(output.squeeze(1));  // [N,E]
    auto logits   = torch::mm(fused_v, bone_feats.t());     // [N,B]
    auto weights  = torch::softmax(logits, /*dim=*/1);      // per-vertex distribution over bones
    return weights;
  }
};
TORCH_MODULE(CrossAttention);

// Full Model
struct SkinningModelImpl : torch::nn::Module {
  MeshEncoder mesh_enc{nullptr};
  SkeletonEncoder skel_enc{nullptr};
  CrossAttention cross_attn{nullptr};
  torch::nn::Linear bone_proj{nullptr};

  SkinningModelImpl(
      int mesh_in_ch, const std::vector<int>& edge_out_channels, int trans_dim, int bone_in_dim, int bone_embed_dim,
      int nhead = 8, int k = 16
  ) {
    mesh_enc =
        register_module("mesh_enc", MeshEncoder(mesh_in_ch, edge_out_channels, trans_dim, nhead, /*num_layers*/ 2, k));
    skel_enc = register_module("skel_enc", SkeletonEncoder(bone_in_dim, bone_embed_dim, /*num_layers*/ 2));
    // Ensure bone_embed_dim == trans_dim; if not, add a learnable projection registered on the module.
    if (bone_embed_dim != trans_dim) {
      bone_proj = register_module("bone_proj", torch::nn::Linear(bone_embed_dim, trans_dim));
    }
    cross_attn = register_module("cross_attn", CrossAttention(trans_dim, nhead));
  }

  // inputs:
  // verts [N,3], normals [N,3], faces [F,3], curvature [N]
  // bones_pos [B,3], bones_parent [B], bones_dir_len [B,4] optional
  torch::Tensor forward(const fbx_load_result& in_fbx_data) {
    auto vfeat = mesh_enc->forward(in_fbx_data);  // [N, E]
    auto bfeat = skel_enc->forward(in_fbx_data);  // [B, E2]
    // If dims mismatch, apply registered projection so parameters are trainable.
    if (bone_proj) {
      bfeat = bone_proj->forward(bfeat);
    }
    // Safety: if still mismatched, fail fast (avoids silent wrong results).
    DOODLE_CHICK(bfeat.size(1) == vfeat.size(1), "bone feature dim must match vertex feature dim");
    auto weights = cross_attn->forward(vfeat, bfeat);  // [N, B]
    return weights;
  }
};
TORCH_MODULE(SkinningModel);

// ----------------------------- Training function --------------------------------
std::vector<fbx_load_result> load_fbx_files(const std::vector<FSys::path>& in_fbx_files) {
  std::vector<fbx_load_result> results;
  for (const auto& fbx_file : in_fbx_files) {
    if (!FSys::exists(fbx_file)) {
      DOODLE_LOG_ERROR("FBX file does not exist: {}", fbx_file.string());
      continue;
    }
    fbx_loader l_loader{fbx_file};
    results.push_back(l_loader.load_fbx());
  }
  return results;
}

class cross_attention_bone_weight::impl {
 public:
  impl() = default;
  SkinningModel model_{nullptr};
};

std::shared_ptr<cross_attention_bone_weight> cross_attention_bone_weight::train(
    const std::vector<FSys::path>& in_fbx_files, const FSys::path& in_output_path
) {
  if (auto l_parent = in_output_path.parent_path(); !FSys::exists(l_parent)) {
    FSys::create_directories(l_parent);
  }
  torch::manual_seed(42);
  torch::Device device(torch::kCPU);
  if (torch::cuda::is_available()) {
    SPDLOG_WARN("CUDA available. Using GPU.");
    device = torch::Device(torch::kCUDA);
  } else {
    SPDLOG_WARN("Using CPU.");
  }

  DOODLE_CHICK(!in_fbx_files.empty(), "No input FBX files provided for training.");
  auto l_fbx_data           = load_fbx_files(in_fbx_files);

  auto l_ret                = std::make_shared<cross_attention_bone_weight>();
  int epochs                = 250;
  int batch_size            = 1;
  float lr                  = 1e-3;
  int k                     = 16;
  // Model hyperparams
  int mesh_in_ch            = 3 + 3 + 1 + 1;  // verts + normals + curvature + topo_degree(from faces_)
  // In our implementation we concatenated many fields; compute actual in channels dynamically when loading first
  // sample. For simplicity set:
  int trans_dim             = 128;
  std::vector<int> edge_out = {64, 128};  // EdgeConv outputs
  int bone_in_dim           = 3 + 3;      // pos + rel -> may be larger if bones_dir_len used
  int bone_embed_dim        = 128;
  int nhead                 = 8;
  for (auto& l_data : l_fbx_data) {
    l_data.build_face_adjacency(k);
    l_data.compute_curvature();
    l_data.normalize_inputs();
    l_data.compute_bones_dir_len();
    // Move to device
    l_data.to(device);
  }

  // Create model
  SkinningModel model{mesh_in_ch, edge_out, trans_dim, bone_in_dim, bone_embed_dim, nhead, k};
  model->to(device);

  torch::optim::Adam optimizer(model->parameters(), torch::optim::AdamOptions(lr));
  // scheduler optional

  // Training loop: simple epoch over samples (batch_size=1)
  for (int epoch = 1; epoch <= epochs; ++epoch) {
    model->train();
    double epoch_loss = 0.0;
    int count         = 0;
    for (auto& l_data : l_fbx_data) {
      // ensure shapes: target_weights [N, B]
      auto N           = l_data.vertices_.size(0);
      auto B           = l_data.bone_positions_.size(0);
      // forward
      auto pred        = model->forward(l_data);  // [N,B]
      // ensure numeric stability
      pred             = pred.clamp_min(1e-8);
      // compute loss: KL divergence per-vertex: sum p*log(p/q) where p=target, q=pred
      auto target      = l_data.bone_weights_;
      // Safe normalization: some vertices may have all-zero weights; clamp denom to avoid NaNs.
      auto eps         = 1e-8;
      auto pred_norm   = pred / pred.sum(1, true).clamp_min(eps);
      auto target_norm = target / target.sum(1, true).clamp_min(eps);
      auto kl          = (target_norm * ((target_norm + eps).log() - (pred_norm + eps).log())).sum(1).mean();
      // also add L2 between pred and target
      auto mse         = torch::mse_loss(pred_norm, target_norm);
      auto loss        = kl + 0.5 * mse;

      // Diagnostics (print occasionally to avoid spam)
      if (epoch == 1 || epoch == 2 || epoch == 3 || (epoch % 10 == 0)) {
        const auto pred_finite    = torch::isfinite(pred_norm).all().item<bool>();
        const auto target_finite  = torch::isfinite(target_norm).all().item<bool>();
        const auto target_min     = target.min().item<float>();
        const auto target_max     = target.max().item<float>();
        const auto target_row_sum = target.sum(1);
        const auto tsum_min       = target_row_sum.min().item<float>();
        const auto tsum_max       = target_row_sum.max().item<float>();
        const auto tsum_mean      = target_row_sum.mean().item<float>();

        const auto pred_max_mean  = std::get<0>(pred_norm.max(1)).mean().item<float>();
        const auto pred_entropy   = (-(pred_norm * (pred_norm + eps).log()).sum(1)).mean().item<float>();

        const auto v_abs_max      = l_data.vertices_.abs().max().item<float>();
        const auto n_abs_max      = l_data.normals_.abs().max().item<float>();
        const auto curv_min       = l_data.curvature_.min().item<float>();
        const auto curv_max       = l_data.curvature_.max().item<float>();
        const auto deg_min        = l_data.topo_degree_.min().item<float>();
        const auto deg_max        = l_data.topo_degree_.max().item<float>();

        SPDLOG_WARN(
            "[epoch {}/{}] N={} B={} loss={} (kl={}, mse={}) finite(pred/target)={}/{} | "
            "target[min,max]=[{:.3g},{:.3g}] "
            "target_row_sum[min,max,mean]=[{:.3g},{:.3g},{:.3g}] | pred[max_mean]={:.3g} pred[entropy]={:.3g} | "
            "in: |v|_max={:.3g} |n|_max={:.3g} curv[min,max]=[{:.3g},{:.3g}] deg[min,max]=[{:.3g},{:.3g}]",
            epoch, epochs, N, B, loss.item<double>(), kl.item<double>(), mse.item<double>(), pred_finite, target_finite,
            target_min, target_max, tsum_min, tsum_max, tsum_mean, pred_max_mean, pred_entropy, v_abs_max, n_abs_max,
            curv_min, curv_max, deg_min, deg_max
        );
      }

      optimizer.zero_grad();
      loss.backward();
      optimizer.step();

      epoch_loss += loss.item<double>();
      count++;
    }
    SPDLOG_WARN("[epoch {}/{}] avg loss: {}", epoch, epochs, (epoch_loss / std::max(1, count)));

    // optional checkpoint
    if (epoch % 10 == 0) {
      auto l_file_name = in_output_path.parent_path() / fmt::format("{}_epoch_{}.pt", in_output_path.stem(), epoch);
      torch::save(model, l_file_name.generic_string());
      SPDLOG_WARN("Saved checkpoint: {}", l_file_name);
    }
  }

  // final save
  torch::save(model, in_output_path.generic_string());
  SPDLOG_WARN("Saved final model: {}", in_output_path);
  l_ret->pimpl_->model_ = model;
  return l_ret;
}

void cross_attention_bone_weight::load_model(const FSys::path& in_model_path) {
  pimpl_ = std::make_shared<impl>();
  torch::load(pimpl_->model_, in_model_path.generic_string());
}

void cross_attention_bone_weight::predict_by_fbx(
    const FSys::path& in_fbx_path, const FSys::path& out_fbx_path, logger_ptr_raw in_logger
) {
  DOODLE_CHICK(pimpl_->model_, "Model not loaded. Cannot predict.");

  fbx_loader l_loader{in_fbx_path};
  auto l_data = l_loader.load_fbx();
  l_data.build_face_adjacency(16);
  l_data.compute_curvature();
  l_data.normalize_inputs();
  l_data.compute_bones_dir_len();

  // Move to device
  torch::Device device(torch::kCPU);
  if (torch::cuda::is_available()) {
    device = torch::Device(torch::kCUDA);
  }
  l_data.to(device);
  pimpl_->model_->to(device);
  pimpl_->model_->eval();

  // Forward
  auto pred_weights = pimpl_->model_->forward(l_data);  // [N,B]
  pred_weights      = pred_weights.clamp_min(1e-8);
  // Normalize
  pred_weights      = pred_weights / pred_weights.sum(1, true);
}
}  // namespace doodle::ai