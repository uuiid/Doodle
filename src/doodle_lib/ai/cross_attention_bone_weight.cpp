#include "cross_attention_bone_weight.h"

#include <doodle_lib/ai/load_fbx.h>

#include <ATen/core/TensorBody.h>
#include <memory>
#include <torch/types.h>
#include <vector>

namespace doodle::ai {
// -------------------- Utility --------------------
static torch::Tensor load_tensor_from_file(const std::string& path) {
  // We assume path is a saved .pt containing a single tensor
  torch::Tensor t;
  try {
    torch::load(t, path);
  } catch (const c10::Error& e) {
    throw std::runtime_error(std::string("Failed to load tensor: ") + e.what());
  }
  return t;
}

// -------------------- Mesh Encoder --------------------
struct MeshEncoderImpl : torch::nn::Module {
  // Simple point transformer style using MHA over local neighborhoods optionally
  int in_dim;
  int d_model;
  int nhead;
  torch::nn::Linear input_proj{nullptr};
  torch::nn::MultiheadAttention mha{nullptr};
  torch::nn::Sequential mlp{nullptr};

  MeshEncoderImpl(int in_dim_, int d_model_, int nhead_) : in_dim(in_dim_), d_model(d_model_), nhead(nhead_) {
    input_proj = register_module("input_proj", torch::nn::Linear(in_dim, d_model));
    mha = register_module("mha", torch::nn::MultiheadAttention(torch::nn::MultiheadAttentionOptions(d_model, nhead)));
    mlp = register_module(
        "mlp", torch::nn::Sequential(
                   torch::nn::Linear(d_model, d_model * 2), torch::nn::ReLU(), torch::nn::Linear(d_model * 2, d_model)
               )
    );
  }

  // verts: [N,3], normals: [N,3]
  torch::Tensor forward(torch::Tensor verts, torch::Tensor normals) {
    // Concatenate features
    auto x        = torch::cat({verts, normals}, -1);  // [N,6]
    x             = input_proj->forward(x);            // [N, d_model]
    // MHA in libtorch expects [seq_len, batch, embed]
    auto x_t      = x.unsqueeze(1).transpose(0, 1);  // [1, N, d_model]
    // self-attention: query/key/value all x_t
    auto attn_out = std::get<0>(mha->forward(x_t, x_t, x_t));  // [1, N, d_model]
    attn_out      = attn_out.transpose(0, 1).squeeze(1);       // [N, d_model]
    auto out      = mlp->forward(attn_out + x);
    return out;  // [N, d_model]
  }
};
TORCH_MODULE(MeshEncoder);

// -------------------- Skeleton Encoder (Tree-GNN) --------------------
struct SkeletonEncoderImpl : torch::nn::Module {
  int in_dim;  // bone input dim (pos + optional dir/len / embed)
  int d_model;
  torch::nn::Linear node_proj{nullptr};
  torch::nn::Linear msg_lin{nullptr};
  torch::nn::Sequential readout{nullptr};

  SkeletonEncoderImpl(int in_dim_, int d_model_) : in_dim(in_dim_), d_model(d_model_) {
    node_proj = register_module("node_proj", torch::nn::Linear(in_dim, d_model));
    msg_lin   = register_module("msg_lin", torch::nn::Linear(d_model, d_model));
    readout   = register_module(
        "readout", torch::nn::Sequential(
                       torch::nn::Linear(d_model, d_model), torch::nn::ReLU(), torch::nn::Linear(d_model, d_model)
                   )
    );
  }

  // bones_pos: [M,3]
  // parent_idx: [M] int64, -1 for root
  torch::Tensor forward(torch::Tensor bones_pos, torch::Tensor parent_idx) {
    auto M           = bones_pos.size(0);
    // compute direction/length
    auto parent_mask = (parent_idx >= 0);
    auto child_pos   = bones_pos;
    auto parent_pos  = torch::zeros_like(bones_pos);
    if (parent_mask.any().item<bool>()) {
      // fill parent_pos where parent>=0
      for (int64_t i = 0; i < M; ++i) {
        auto p = parent_idx[i].item<int64_t>();
        if (p >= 0) parent_pos[i] = bones_pos[p];
      }
    }
    auto dir        = child_pos - parent_pos;  // [M,3], root will be child_pos
    auto length     = torch::norm(dir, 2, /*dim=*/1, /*keepdim=*/true);
    auto dir_norm   = torch::where(length > 1e-6, dir / (length + 1e-12), torch::zeros_like(dir));
    auto node_input = torch::cat({bones_pos, dir_norm, length}, -1);  // [M,7]
    auto h          = node_proj->forward(node_input);                 // [M,d_model]

    // simple tree message passing: message from parent to child and child to parent
    // we'll do T iterations
    int T           = 3;
    for (int t = 0; t < T; ++t) {
      auto h_new = h.clone();
      // aggregate parent->child
      for (int64_t i = 0; i < M; ++i) {
        auto p            = parent_idx[i].item<int64_t>();
        torch::Tensor agg = torch::zeros({d_model}, h.options());
        if (p >= 0) {
          agg += msg_lin->forward(h[p]);
        }
        // children of i: collect
        std::vector<int64_t> children;
        for (int64_t j = 0; j < M; ++j)
          if (parent_idx[j].item<int64_t>() == i) children.push_back(j);
        if (!children.empty()) {
          for (auto c : children) agg += msg_lin->forward(h[c]);
        }
        h_new[i] = torch::relu(h[i] + agg);
      }
      h = h_new;
    }

    auto out = readout->forward(h);  // [M,d_model]
    return out;
  }
};
TORCH_MODULE(SkeletonEncoder);

// -------------------- Cross-Attention Skin Predictor --------------------
struct CrossAttentionSkinImpl : torch::nn::Module {
  int v_dim, b_dim;
  torch::nn::Linear q_lin{nullptr}, k_lin{nullptr}, v_lin{nullptr}, out_lin{nullptr};

  CrossAttentionSkinImpl(int v_dim_, int b_dim_, int out_dim) : v_dim(v_dim_), b_dim(b_dim_) {
    q_lin   = register_module("q_lin", torch::nn::Linear(v_dim, out_dim));
    k_lin   = register_module("k_lin", torch::nn::Linear(b_dim, out_dim));
    v_lin   = register_module("v_lin", torch::nn::Linear(b_dim, out_dim));
    out_lin = register_module("out_lin", torch::nn::Linear(out_dim, out_dim));
  }

  // v_feats: [N, v_dim]
  // b_feats: [M, b_dim]
  // return weights [N,M]
  torch::Tensor forward(torch::Tensor v_feats, torch::Tensor b_feats) {
    auto N       = v_feats.size(0);
    auto M       = b_feats.size(0);
    auto Q       = q_lin->forward(v_feats);  // [N, D]
    auto K       = k_lin->forward(b_feats);  // [M, D]
    auto V       = v_lin->forward(b_feats);  // [M, D]
    // compute attention scores: [N, M]
    auto scores  = torch::mm(Q, K.transpose(0, 1));
    // scale
    scores       = scores / std::sqrt((double)Q.size(1));
    auto attn    = torch::softmax(scores, /*dim=*/1);  // across bones
    // for optional decoded features: out = attn @ V  -> [N,D]
    auto dec     = torch::mm(attn, V);
    auto out     = out_lin->forward(dec);
    // To get final weights ensure non-negative and rows sum to 1
    auto weights = attn;  // already softmaxed (N x M)
    return weights;
  }
};
TORCH_MODULE(CrossAttentionSkin);

// -------------------- Full Model --------------------
struct SkinningModelImpl : torch::nn::Module {
  MeshEncoder mesh_enc{nullptr};
  SkeletonEncoder skel_enc{nullptr};
  CrossAttentionSkin cross_attn{nullptr};

  SkinningModelImpl(
      int mesh_in_dim, int mesh_feat_dim, int mesh_nhead, int skel_in_dim, int skel_feat_dim, int cross_out_dim
  ) {
    mesh_enc   = register_module("mesh_enc", MeshEncoder(mesh_in_dim, mesh_feat_dim, mesh_nhead));
    skel_enc   = register_module("skel_enc", SkeletonEncoder(skel_in_dim, skel_feat_dim));
    cross_attn = register_module("cross_attn", CrossAttentionSkin(mesh_feat_dim, skel_feat_dim, cross_out_dim));
  }

  // verts [N,3], normals [N,3], bones_pos [M,3], parent_idx [M]
  torch::Tensor forward(torch::Tensor verts, torch::Tensor normals, torch::Tensor bones_pos, torch::Tensor parent_idx) {
    auto v_feats = mesh_enc->forward(verts, normals);         // [N, mesh_feat_dim]
    auto b_feats = skel_enc->forward(bones_pos, parent_idx);  // [M, skel_feat_dim]
    auto weights = cross_attn->forward(v_feats, b_feats);     // [N,M]
    return weights;
  }
};
TORCH_MODULE(SkinningModel);

// -------------------- Dataset --------------------
struct SkinningDataset : torch::data::Dataset<SkinningDataset> {
  std::vector<std::string> files;

  SkinningDataset(const std::string& folder) {
    for (auto& p : FSys::directory_iterator(folder)) {
      if (p.path().extension() == ".pt") files.push_back(p.path().string());
    }
    if (files.empty()) throw std::runtime_error("No .pt dataset files found in folder: " + folder);
  }

  torch::data::Example<> get(size_t index) override {
    auto path = files.at(index);
    // load a dictionary from path
    std::unordered_map<std::string, torch::Tensor> sample;
    try {
      // torch::load(sample, path);
    } catch (...) {
      throw std::runtime_error("Failed to load sample: " + path);
    }
    auto verts      = sample.at("verts");
    auto normals    = sample.at("normals");
    auto bones_pos  = sample.at("bones_pos");
    auto parent_idx = sample.at("bones_parent");
    auto weights    = sample.at("weights");
    // We'll pack inputs into a single tensor by concatenation is NOT ideal. Instead
    // return a tuple via Example: data=packed tensor, target=weights. We'll pack metadata sizes in first row.
    // Simpler approach: return data as flattened vector [verts,normals,bones_pos,parent_idx sizes],
    // but Dataset consumer below will instead load the files directly by path index. So here we return path as a tensor
    // index placeholder. We'll instead return an Example with data = torch::tensor({(int64)index}) and handle loading
    // in collate.
    auto idx_tensor = torch::tensor({(int64_t)index}, torch::kInt64);
    return {idx_tensor, weights};
  }

  torch::optional<size_t> size() const override { return files.size(); }
};

// Custom collate function: load actual sample tensors by index
struct Collate {
  std::vector<std::string> files;
  Collate(const std::vector<std::string>& files_) : files(files_) {}
  torch::data::Example<> operator()(std::vector<torch::data::Example<>> batch) {
    // single element batches only supported in this simple collate
    auto idx  = batch[0].data.item<int64_t>();
    auto path = files.at((size_t)idx);
    std::unordered_map<std::string, torch::Tensor> sample;
    // torch::load(sample, path);
    auto verts      = sample.at("verts");
    auto normals    = sample.at("normals");
    auto bones_pos  = sample.at("bones_pos");
    auto parent_idx = sample.at("bones_parent");
    auto weights    = sample.at("weights");
    // We'll store all tensors into a single concatenated tensor via dict packing is not available.
    // Instead, as a pragmatic approach, set data = torch::stack of metadata pointers is not feasible.
    // So use target as weights and rely on external loader in training loop to fetch full sample.
    // Here we return data as verts flattened for compatibility; training loop will re-load by index.
    auto data       = verts;  // placeholder
    return {data, weights};
  }
};

// -------------------- Training Loop --------------------
int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <dataset_folder> <output_model.pt> [--epochs N] [--batch 1]" << std::endl;
    return 1;
  }
  std::string dataset_folder = argv[1];
  std::string out_model      = argv[2];
  int epochs                 = 50;
  int batch_size             = 1;  // variable-sized meshes, use batch 1
  for (int i = 3; i < argc; ++i) {
    std::string s = argv[i];
    if (s == "--epochs" && i + 1 < argc) {
      epochs = std::stoi(argv[++i]);
    }
    if (s == "--batch" && i + 1 < argc) {
      batch_size = std::stoi(argv[++i]);
    }
  }

  // Hyperparams & model
  int mesh_in_dim   = 6;  // xyz + normals
  int mesh_feat_dim = 256;
  int mesh_nhead    = 8;
  int skel_in_dim   = 7;  // pos(3) + dir(3) + length(1)
  int skel_feat_dim = 128;
  int cross_out_dim = 128;

  auto device       = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;
  std::cout << "Using device: " << (device == torch::kCUDA ? "CUDA" : "CPU") << std::endl;

  SkinningModel model{mesh_in_dim, mesh_feat_dim, mesh_nhead, skel_in_dim, skel_feat_dim, cross_out_dim};
  model->to(device);

  // Simple MSE loss between predicted weights and GT
  torch::optim::Adam optimizer(model->parameters(), torch::optim::AdamOptions(1e-4));

  // Collect dataset files
  std::vector<std::string> files;
  for (auto& p : FSys::directory_iterator(dataset_folder))
    if (p.path().extension() == ".pt") files.push_back(p.path().string());
  if (files.empty()) {
    std::cerr << "No .pt files in dataset folder" << std::endl;
    return 1;
  }

  // We'll use a simple loop by file (batch_size assumed 1)
  for (int epoch = 1; epoch <= epochs; ++epoch) {
    model->train();
    double epoch_loss = 0.0;
    int count         = 0;
    // shuffle
    std::shuffle(files.begin(), files.end(), std::default_random_engine(epoch));
    for (auto& path : files) {
      std::unordered_map<std::string, torch::Tensor> sample;
      // torch::load(sample, path);
      auto verts      = sample.at("verts").to(device);
      auto normals    = sample.at("normals").to(device);
      auto bones_pos  = sample.at("bones_pos").to(device);
      auto parent_idx = sample.at("bones_parent").to(device);
      auto gt_weights = sample.at("weights").to(device);

      optimizer.zero_grad();
      auto pred = model->forward(verts, normals, bones_pos, parent_idx);  // [N,M]
      // If predicted has different shape due to rounding, resize
      if (pred.sizes() != gt_weights.sizes()) {
        // try to crop or pad
        auto N = gt_weights.size(0), M = gt_weights.size(1);
        pred = pred.index({torch::indexing::Slice(0, N), torch::indexing::Slice(0, M)});
      }
      auto loss = torch::mse_loss(pred, gt_weights);
      loss.backward();
      optimizer.step();

      epoch_loss += loss.item<double>();
      ++count;
      if (count % 10 == 0)
        std::cout << "Epoch " << epoch << " step " << count << " loss " << loss.item<double>() << "\n";
    }
    std::cout << "Epoch " << epoch << " avg loss " << (epoch_loss / count) << std::endl;
    // save checkpoint
    torch::save(model, out_model + "_epoch" + std::to_string(epoch) + ".pt");
  }

  // final save
  torch::save(model, out_model);
  std::cout << "Training finished. Model saved to " << out_model << std::endl;
  return 0;
}
std::shared_ptr<cross_attention_bone_weight> cross_attention_bone_weight::train(
    const std::vector<FSys::path>& in_fbx_files, const FSys::path& in_output_path
) {
  auto l_ret = std::make_shared<cross_attention_bone_weight>();

  return l_ret;
}

}  // namespace doodle::ai