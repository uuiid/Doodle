#include "cross_attention_bone_weight.h"

#include "doodle_core/exception/exception.h"

#include <doodle_lib/ai/load_fbx.h>

#include <ATen/core/TensorBody.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <torch/types.h>
#include <vector>

namespace doodle::ai {

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <torch/torch.h>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

// ----------------------------- Utility functions --------------------------------

// Compute pairwise squared distances, returns [N, M]
torch::Tensor pairwise_distances(const torch::Tensor& a, const torch::Tensor& b) {
  // a: [N, D], b: [M, D]
  // uses (x-y)^2 = x^2 + y^2 - 2xy
  auto a2    = a.pow(2).sum(1).unsqueeze(1);  // [N,1]
  auto b2    = b.pow(2).sum(1).unsqueeze(0);  // [1,M]
  auto prod  = torch::mm(a, b.t());           // [N,M]
  auto dist2 = a2 + b2 - 2 * prod;
  return dist2.clamp_min(0);
}

// kNN brute force: returns indices [N, k]
torch::Tensor knn_indices(const torch::Tensor& points, int k) {
  // points: [N, D]
  auto dist2 = pairwise_distances(points, points);  // [N,N]
  // set diagonal large so not pick self if desired; but for EdgeConv often include self - here exclude self:
  auto N     = points.size(0);
  dist2 += torch::eye(N, dist2.options()) * 1e9;
  auto topk = std::get<1>(dist2.topk(k, /*dim=*/1, /*largest=*/false));
  return topk;  // [N, k]
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

  // x: [N, C] ; pos: [N, 3]
  torch::Tensor forward(const torch::Tensor& x, const torch::Tensor& pos) {
    auto idx   = knn_indices(pos, k);                 // [N,k]
    auto neigh = gather_neighbors(x, idx);            // [N,k,C]
    auto xi    = x.unsqueeze(1).expand({-1, k, -1});  // [N,k,C]
    auto feat  = torch::cat({xi, neigh - xi}, -1);    // [N,k,2C]
    auto out   = torch::relu(mlp->forward(feat));     // broadcasting linear applies to last dim? Need to flatten
    // linear expects [..., in_features]; apply by reshaping
    auto N = feat.size(0), K = feat.size(1);
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

  MeshEncoderImpl(
      int in_ch, const std::vector<int>& edge_out_channels, int trans_dim, int nhead = 8, int num_layers = 2, int k = 16
  ) {
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

  // verts: [N,3], normals: [N,3], curvature: [N,1], degree:[N,1], normal_dev:[N,1]
  torch::Tensor forward(
      const torch::Tensor& verts, const torch::Tensor& normals, const torch::Tensor& curvature,
      const torch::Tensor& degree, const torch::Tensor& normal_dev
  ) {
    // build initial feature
    auto feat = torch::cat(
        {verts, normals, curvature.unsqueeze(1), degree.unsqueeze(1), normal_dev.unsqueeze(1)}, -1
    );  // [N, F]
    auto x = feat;
    for (size_t i = 0; i < edge_layers.size(); ++i) {
      x = edge_layers[i]->forward(x, verts);
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
  // verts [N,3], normals [N,3], curvature [N], degree [N], normal_dev [N]
  // bones_pos [B,3], bones_parent [B], bones_dir_len [B,4] optional
  torch::Tensor forward(
      const torch::Tensor& verts, const torch::Tensor& normals, const torch::Tensor& curvature,
      const torch::Tensor& degree, const torch::Tensor& normal_dev, const torch::Tensor& bones_pos,
      const torch::Tensor& bones_parent, const torch::Tensor& bones_dir_len = torch::Tensor()
  ) {
    auto vfeat = mesh_enc->forward(verts, normals, curvature, degree, normal_dev);  // [N, E]
    auto bfeat = skel_enc->forward(bones_pos, bones_parent, bones_dir_len);         // [B, E2]
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

// ----------------------------- Training loop -----------------------------------

struct Sample {
  torch::Tensor verts, normals, curvature, degree, normal_dev;
  torch::Tensor bones_pos, bones_parent, bones_dir_len;
  torch::Tensor target_weights;
};

Sample load_sample_from_folder(const fs::path& dir, torch::Device device) {
  auto load_tensor = [&](const fs::path& p) -> torch::Tensor {
    torch::Tensor t;
    torch::load(t, p.string());
    return t.to(device);
  };
  Sample s;
  s.verts        = load_tensor(dir / "vertices.pt");
  s.normals      = load_tensor(dir / "normals.pt");
  s.curvature    = load_tensor(dir / "curvature.pt");
  s.degree       = load_tensor(dir / "degree.pt");
  s.normal_dev   = load_tensor(dir / "normal_dev.pt");
  s.bones_pos    = load_tensor(dir / "bones_pos.pt");
  s.bones_parent = load_tensor(dir / "bones_parent.pt").to(torch::kLong).to(device);
  if (fs::exists(dir / "bones_dir_len.pt"))
    s.bones_dir_len = load_tensor(dir / "bones_dir_len.pt");
  else
    s.bones_dir_len = torch::Tensor();
  s.target_weights = load_tensor(dir / "target_weights.pt");
  return s;
}

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
  auto l_ret           = std::make_shared<cross_attention_bone_weight>();

  std::string data_dir = "../data";
  int epochs           = 100;
  int batch_size       = 1;
  float lr             = 1e-3;
  int k                = 16;

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
  int mesh_in_ch            = 3 + 3 + 3;  // we will pack curvature/degree/normal_dev into extra dims; adapt below
  // In our implementation we concatenated many fields; compute actual in channels dynamically when loading first
  // sample. For simplicity set:
  int trans_dim             = 128;
  std::vector<int> edge_out = {64, 128};  // EdgeConv outputs
  int bone_in_dim           = 3 + 3;      // pos + rel -> may be larger if bones_dir_len used
  int bone_embed_dim        = 128;
  int nhead                 = 8;

  // Create model
  SkinningModel model{/*mesh_in_ch*/ 8, edge_out, trans_dim, bone_in_dim, bone_embed_dim, nhead, k};
  model->to(device);

  torch::optim::Adam optimizer(model->parameters(), torch::optim::AdamOptions(lr));
  // scheduler optional

  // Training loop: simple epoch over samples (batch_size=1)
  for (int epoch = 1; epoch <= epochs; ++epoch) {
    model->train();
    double epoch_loss = 0.0;
    int count         = 0;
    for (auto& l_data : l_fbx_data) {
      // load sample
      Sample s;
      // ensure shapes: target_weights [N, B]
      auto N    = s.verts.size(0);
      auto B    = s.bones_pos.size(0);
      // forward
      auto pred = model->forward(
          l_data.vertices_, l_data.normals_, l_data.curvature_, l_data.degree_, l_data.normal_deviation_,
          l_data.bone_positions_, l_data.bone_parents_, l_data.bones_dir_len_
      );  // [N,B]
      // ensure numeric stability
      pred             = pred.clamp_min(1e-8);
      // compute loss: KL divergence per-vertex: sum p*log(p/q) where p=target, q=pred
      auto target      = s.target_weights;
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