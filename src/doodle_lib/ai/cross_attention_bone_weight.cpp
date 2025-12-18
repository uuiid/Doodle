#include "cross_attention_bone_weight.h"

#include "doodle_core/exception/exception.h"

#include <doodle_lib/ai/load_fbx.h>

#include <ATen/core/TensorBody.h>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>
#include <torch/torch.h>
#include <torch/types.h>
#include <vector>

namespace doodle::ai {

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

struct FaceAdjacency {
  torch::Tensor neighbor_idx;  // [N, k] int64
  torch::Tensor topo_degree;   // [N] float32
};

// Build per-vertex fixed-size neighbor indices from triangle faces.
// - faces: [F,3] int64 (can be on any device)
// - neighbor_idx: [N,k] int64 on `device`
// - topo_degree: [N] float32 on `device` (unique neighbor count)
FaceAdjacency build_face_adjacency(
    const torch::Tensor& faces, int64_t num_verts, int64_t k, const torch::Device& device
) {
  DOODLE_CHICK(faces.defined(), "faces tensor is undefined");
  DOODLE_CHICK(faces.dim() == 2, "faces must be 2D [F,3]");
  DOODLE_CHICK(faces.size(1) >= 3, "faces must have at least 3 columns");
  DOODLE_CHICK(num_verts > 0, "num_verts must be > 0");
  DOODLE_CHICK(k > 0, "k must be > 0");

  // Build adjacency on CPU for simplicity/stability.
  auto faces_cpu = faces.to(torch::kCPU).to(torch::kInt64).contiguous();

  std::vector<std::vector<int64_t>> adj(static_cast<size_t>(num_verts));
  auto acc        = faces_cpu.accessor<int64_t, 2>();
  const int64_t F = faces_cpu.size(0);
  for (int64_t f = 0; f < F; ++f) {
    const int64_t a = acc[f][0];
    const int64_t b = acc[f][1];
    const int64_t c = acc[f][2];
    if (a < 0 || b < 0 || c < 0) {
      continue;
    }
    if (a >= num_verts || b >= num_verts || c >= num_verts) {
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

  auto idx_cpu = torch::empty({num_verts, k}, torch::TensorOptions().dtype(torch::kInt64).device(torch::kCPU));
  auto deg_cpu = torch::empty({num_verts}, torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU));
  auto idx_acc = idx_cpu.accessor<int64_t, 2>();
  auto deg_acc = deg_cpu.accessor<float, 1>();

  for (int64_t v = 0; v < num_verts; ++v) {
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

  FaceAdjacency out;
  out.neighbor_idx = idx_cpu.to(device);
  out.topo_degree  = deg_cpu.to(device);
  return out;
}

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
  torch::Tensor forward(
      const torch::Tensor& verts, const torch::Tensor& normals, const torch::Tensor& faces,
      const torch::Tensor& curvature
  ) {
    auto adj         = build_face_adjacency(faces, verts.size(0), k, verts.device());
    // topo_degree is scalar connectivity feature per-vertex, derived from faces_
    auto topo_degree = adj.topo_degree;

    // build initial feature
    auto feat        = torch::cat(
        {verts, normals, curvature.unsqueeze(1), topo_degree.unsqueeze(1)}, -1
    );  // [N, F]
    auto x = feat;
    for (size_t i = 0; i < edge_layers.size(); ++i) {
      x = edge_layers[i]->forward(x, adj.neighbor_idx);
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
  torch::Tensor forward(
      const torch::Tensor& bones_pos, const torch::Tensor& parent_idx,
      const torch::Tensor& bones_dir_len = torch::Tensor()
  ) {
    // initial features: pos + (dir,len)
    torch::Tensor feat;
    if (bones_dir_len.defined()) {
      feat = torch::cat({bones_pos, bones_dir_len}, -1);
    } else {
      // Use relative to parent vector if parent exists
      int B    = bones_pos.size(0);
      auto rel = torch::zeros({B, 3}, bones_pos.options());
      for (int i = 0; i < B; ++i) {
        int p = parent_idx[i].item<int>();
        if (p >= 0)
          rel[i] = bones_pos[i] - bones_pos[p];
        else
          rel[i] = bones_pos[i];
      }
      feat = torch::cat({bones_pos, rel}, -1);
    }
    auto h = torch::relu(embed->forward(feat));
    for (auto& l : layers) {
      h = l->forward(h, parent_idx);
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
    auto attn_weights =
        std::get<1>(attn_out);  // [N, B] ? shape: [batch*num_heads, tgt_len, src_len]?  C++ API may return [N, B]
    // To get per-vertex weight distribution over bones, compute similarity Q.K^T normalized by softmax:
    auto logits  = torch::mm(vertex_feats, bone_feats.t());  // [N, B]
    auto weights = torch::softmax(logits, /*dim=*/1);        // sum over bones =1 for each vertex
    return weights;
  }
};
TORCH_MODULE(CrossAttention);

// Full Model
struct SkinningModelImpl : torch::nn::Module {
  MeshEncoder mesh_enc{nullptr};
  SkeletonEncoder skel_enc{nullptr};
  CrossAttention cross_attn{nullptr};

  SkinningModelImpl(
      int mesh_in_ch, const std::vector<int>& edge_out_channels, int trans_dim, int bone_in_dim, int bone_embed_dim,
      int nhead = 8, int k = 16
  ) {
    mesh_enc =
        register_module("mesh_enc", MeshEncoder(mesh_in_ch, edge_out_channels, trans_dim, nhead, /*num_layers*/ 2, k));
    skel_enc = register_module("skel_enc", SkeletonEncoder(bone_in_dim, bone_embed_dim, /*num_layers*/ 2));
    // Ensure bone_embed_dim == trans_dim or project accordingly; we'll create a linear projection if needed
    if (bone_embed_dim != trans_dim) {
      // For simplicity we assume same; otherwise add linear projection in cross-attn flow
    }
    cross_attn = register_module("cross_attn", CrossAttention(trans_dim, nhead));
  }

  // inputs:
  // verts [N,3], normals [N,3], faces [F,3], curvature [N]
  // bones_pos [B,3], bones_parent [B], bones_dir_len [B,4] optional
  torch::Tensor forward(
      const torch::Tensor& verts, const torch::Tensor& normals, const torch::Tensor& faces,
      const torch::Tensor& curvature, const torch::Tensor& bones_pos,
      const torch::Tensor& bones_parent, const torch::Tensor& bones_dir_len = torch::Tensor()
  ) {
    auto vfeat = mesh_enc->forward(verts, normals, faces, curvature);  // [N, E]
    auto bfeat = skel_enc->forward(bones_pos, bones_parent, bones_dir_len);        // [B, E2]
    // if dims mismatch, linear project
    if (bfeat.size(1) != vfeat.size(1)) {
      // add projection
      auto proj = torch::nn::Linear(bfeat.size(1), vfeat.size(1));
      bfeat     = proj->forward(bfeat);
    }
    auto weights = cross_attn->forward(vfeat, bfeat);  // [N, B]
    return weights;
  }
};
TORCH_MODULE(SkinningModel);

// ----------------------------- Training function --------------------------------
std::vector<fbx_load_result> load_fbx_files(const std::vector<FSys::path>& in_fbx_files) {
  std::vector<fbx_load_result> results;
  for (const auto& fbx_file : in_fbx_files) {
    fbx_loader l_loader{fbx_file};
    results.push_back(l_loader.load_fbx());
  }
  return results;
}

std::shared_ptr<cross_attention_bone_weight> cross_attention_bone_weight::train(
    const std::vector<FSys::path>& in_fbx_files, const FSys::path& in_output_path
) {
  auto l_ret     = std::make_shared<cross_attention_bone_weight>();
  int epochs     = 100;
  int batch_size = 1;
  float lr       = 1e-3;
  int k          = 16;

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
      auto N    = l_data.vertices_.size(0);
      auto B    = l_data.bone_positions_.size(0);
      // forward
      auto pred = model->forward(
          l_data.vertices_, l_data.normals_, l_data.faces_, l_data.curvature_,
          l_data.bone_positions_, l_data.bone_parents_, l_data.bones_dir_len_
      );  // [N,B]
      // ensure numeric stability
      pred             = pred.clamp_min(1e-8);
      // compute loss: KL divergence per-vertex: sum p*log(p/q) where p=target, q=pred
      auto target      = l_data.bone_weights_;
      // target may contain zeros, add eps
      auto pred_norm   = pred / pred.sum(1, true);
      auto target_norm = target / target.sum(1, true);
      auto eps         = 1e-8;
      auto kl          = (target_norm * ((target_norm + eps).log() - (pred_norm + eps).log())).sum(1).mean();
      // also add L2 between pred and target
      auto mse         = torch::mse_loss(pred_norm, target_norm);
      auto loss        = kl + 0.5 * mse;
      optimizer.zero_grad();
      loss.backward();
      optimizer.step();

      epoch_loss += loss.item<double>();
      count++;
    }
    SPDLOG_WARN("[epoch {}/{}] avg loss: {}", epoch, epochs, (epoch_loss / std::max(1, count)));

    // optional checkpoint
    if (epoch % 10 == 0) {
      auto l_file_name = in_output_path / fmt::format("model_epoch_{}.pt", epoch);
      torch::save(model, l_file_name.generic_string());
      SPDLOG_WARN("Saved checkpoint: {}", l_file_name);
    }
  }

  // final save
  auto final_file_name = in_output_path / "model_final.pt";
  torch::save(model, final_file_name.generic_string());
  SPDLOG_WARN("Saved final model: {}", final_file_name);
  return l_ret;
}

}  // namespace doodle::ai