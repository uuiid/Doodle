#include "bone_weight_inference.h"

#include <mimalloc.h>

#include <ATen/core/Reduction.h>
#include <range/v3/action/sort.hpp>

#pragma comment(linker, "/include:mi_version")

#include <torch/csrc/api/include/torch/torch.h>
namespace doodle::ai {

struct GraphSample {
  // node features: [num_nodes, in_dim]
  torch::Tensor x;
  // normalized adjacency: [num_nodes, num_nodes] (float)
  // we will use dense adjacency for simplicity
  torch::Tensor adj;
  // label distribution per node: [num_nodes, num_bones]
  torch::Tensor y;
  // optional: node mask or other metadata
};

torch::Tensor normalize_adjacency(const torch::Tensor& A) {
  // A: [N, N], float
  auto I            = torch::eye(A.size(0), A.options());
  auto A_hat        = A + I;
  auto deg          = A_hat.sum(1);  // [N]
  // avoid div by zero
  auto deg_inv_sqrt = torch::pow(deg + 1e-12, -0.5);
  auto D_inv_sqrt   = torch::diag(deg_inv_sqrt);
  auto norm         = torch::matmul(torch::matmul(D_inv_sqrt, A_hat), D_inv_sqrt);
  return norm;
}

// Example: build node features from vertices & bone positions
// vertices: [N,3], bone_positions: [B,3], faces: [F,3] (optional)
GraphSample build_sample_from_mesh(
    const torch::Tensor& vertices,  // [N,3]
    const std::vector<std::string>& bone_names,
    const torch::Tensor& bone_positions,  // [B,3]
    const torch::Tensor& faces,           // [F,3] or empty
    const torch::Tensor& weights          // [N,B] ground truth distribution
) {
  int64_t N       = vertices.size(0);
  int64_t B       = bone_positions.size(0);

  // 1) adjacency from faces (undirected)
  auto options    = torch::TensorOptions().dtype(torch::kFloat32);
  torch::Tensor A = torch::zeros({N, N}, options);
  if (faces.numel() > 0) {
    auto f_accessor = faces.accessor<int64_t, 2>();
    for (int64_t i = 0; i < faces.size(0); ++i) {
      int64_t a = f_accessor[i][0];
      int64_t b = f_accessor[i][1];
      int64_t c = f_accessor[i][2];
      A[a][b]   = 1;
      A[b][a]   = 1;
      A[b][c]   = 1;
      A[c][b]   = 1;
      A[c][a]   = 1;
      A[a][c]   = 1;
    }
  } else {
    // fallback: fully connected? or k-nearest; here we connect nothing (isolated)
  }

  // 2) feature design: [vx, vy, vz] + distances to K nearest bones
  int K                  = std::min<int>(4, (int)B);
  torch::Tensor v_exp    = vertices.unsqueeze(1).expand({N, B, 3});        // [N,B,3]
  torch::Tensor b_exp    = bone_positions.unsqueeze(0).expand({N, B, 3});  // [N,B,3]
  torch::Tensor dists    = torch::norm(v_exp - b_exp, 2, -1);              // [N,B]
  // take K smallest distances and also their values
  auto sorted            = std::get<0>(dists.sort(1, /*descending=*/false));  // [N,B] sorted distances
  torch::Tensor closest  = sorted.slice(1, 0, K);                             // [N,K]
  // feature concat
  torch::Tensor x        = torch::cat({vertices, closest}, /*dim=*/1);  // [N, 3+K]

  // 3) normalize adjacency
  torch::Tensor adj_norm = normalize_adjacency(A);

  GraphSample s;
  s.x   = x;
  s.adj = adj_norm;
  s.y   = weights;
  return s;
}

// ============= Dataset (simple file-based dataset) =============
// For simplicity, we assume each sample is stored as a .pt file (torch::save)
// with a dict: { "x": Tensor, "adj": Tensor, "y": Tensor }
// We will provide a load_sample function to read these.
class GraphDataset : public torch::data::Dataset<GraphDataset> {
 public:
  GraphDataset(const std::vector<std::string>& files) : files_(files) {}

  torch::data::Example<> get(size_t index) override {
    // load sample file
    auto filename = files_.at(index);
    torch::Tensor x, adj, y;
    // We'll read a scriptmodule-like archive storing tensors under keys "x","adj","y"
    torch::serialize::InputArchive archive;
    archive.load_from(filename);
    archive.read("x", x);
    archive.read("adj", adj);
    archive.read("y", y);

    // For DataLoader compatibility, we will pack x and adj into one tensor via a custom scheme:
    // return data as a tuple-like tensor is inconvenient; instead we'll return a single tensor
    // by concatenating shapes: but simplest for clarity is to return x as data and y as target,
    // and keep adj in a parallel vector in memory (not ideal). For full generality, user can
    // implement a custom collate. Here we cheat: save adj as extra channels in x? To keep clear,
    // we will serialize adj into a 1D tensor appended to x -- BUT for clarity in this example,
    // we will not use DataLoader batching and instead iterate files manually in the training loop.
    // So this get is not used. Still implement minimal.
    return {x, y};
  }

  torch::optional<size_t> size() const override { return files_.size(); }

  // convenience loader for manual iteration
  GraphSample load_sample_file(size_t index) {
    auto filename = files_.at(index);
    torch::Tensor x, adj, y;
    torch::serialize::InputArchive archive;
    archive.load_from(filename);
    archive.read("x", x);
    archive.read("adj", adj);
    archive.read("y", y);
    GraphSample s;
    s.x   = x;
    s.adj = adj;
    s.y   = y;
    return s;
  }

 private:
  std::vector<std::string> files_;
};

// ============= Model Definition =============
// Graph convolution layer (spectral style): H' = norm_adj @ H @ W
struct GraphConvImpl : torch::nn::Module {
  GraphConvImpl(int in_dim, int out_dim) {
    W = register_parameter("W", torch::randn({in_dim, out_dim}) * 0.01);
    b = register_parameter("b", torch::zeros(out_dim));
  }
  torch::Tensor forward(const torch::Tensor& x, const torch::Tensor& adj_norm) {
    // x: [N, Fin], adj_norm: [N,N]
    // out = adj_norm @ x @ W + b
    auto h = torch::matmul(adj_norm, x);  // [N, Fin]
    h      = torch::matmul(h, W) + b;     // [N, Fout]
    return h;
  }
  torch::Tensor W, b;
};
TORCH_MODULE(GraphConv);

// Full model
struct SkinWeightGCNImpl : torch::nn::Module {
  // num_bones can be dynamic. If a positive num_bones is provided to the ctor,
  // we register a parameter matrix `bone_emb` of shape [num_bones, hidden_dim].
  // Alternatively, callers may pass a bone_embeddings tensor to forward()
  // of shape [B, hidden_dim], where B is the number of bones for that sample.
  SkinWeightGCNImpl(int in_channels, int hidden_dim, int num_bones = 0) : gc1(nullptr), gc2(nullptr) {
    gc1  = register_module("gc1", GraphConv(in_channels, hidden_dim));
    gc2  = register_module("gc2", GraphConv(hidden_dim, hidden_dim));
    // global branch
    gfc1 = register_module("gfc1", torch::nn::Linear(in_channels, hidden_dim));
    gfc2 = register_module("gfc2", torch::nn::Linear(hidden_dim, hidden_dim));
    // fusion and final
    fc1  = register_module("fc1", torch::nn::Linear(hidden_dim * 2, hidden_dim));

    // optional: register a bone embedding matrix when num_bones > 0
    if (num_bones > 0) {
      bone_emb = register_parameter("bone_emb", torch::randn({num_bones, hidden_dim}) * 0.01);
    } else {
      bone_emb = torch::Tensor();
    }
  }

  // forward returns log probabilities per node (for KLDiv use)
  // If `bone_embeddings` is provided it will be used (shape [B, hidden_dim]).
  // Otherwise the registered `bone_emb` (if any) will be used.
  torch::Tensor forward(
      const torch::Tensor& x, const torch::Tensor& adj_norm, const torch::Tensor& bone_embeddings = torch::Tensor()
  ) {
    // local branch
    auto l                 = torch::relu(gc1->forward(x, adj_norm));  // [N, H]
    l                      = torch::relu(gc2->forward(l, adj_norm));  // [N, H]

    // global branch: linear on node features -> pooled
    auto g                 = torch::relu(gfc1->forward(x));  // [N, H]
    g                      = torch::relu(gfc2->forward(g));  // [N, H]
    auto g_pool            = g.mean(0, /*keepdim=*/true);    // [1, H]
    // broadcast to nodes
    auto g_bcast           = g_pool.expand({x.size(0), g_pool.size(1)});  // [N,H]

    // concat
    auto feat              = torch::cat({l, g_bcast}, /*dim=*/1);  // [N, 2H]
    auto h                 = torch::relu(fc1->forward(feat));      // [N, H]

    // determine bone embedding matrix
    torch::Tensor used_emb = bone_embeddings.defined() ? bone_embeddings : bone_emb;
    if (!used_emb.defined()) {
      throw std::runtime_error("No bone embeddings provided to forward() and none registered in the model.");
    }
    // used_emb: [B, H]
    auto logits = torch::matmul(h, used_emb.t());  // [N, B]
    return torch::log_softmax(logits, /*dim=*/1);
  }

  GraphConv gc1, gc2;
  torch::nn::Linear gfc1{nullptr}, gfc2{nullptr}, fc1{nullptr};
  // optional bone embedding matrix [B, hidden_dim]
  torch::Tensor bone_emb;
};
TORCH_MODULE(SkinWeightGCN);

// ============= Loss & Metrics =============
torch::Tensor kl_loss_from_logprob(const torch::Tensor& log_pred, const torch::Tensor& target) {
  // target is a prob distribution [N, B], log_pred is log probabilities [N, B]
  // PyTorch kl_div expects input = log-prob, target = prob
  auto loss = torch::kl_div(log_pred, target, /*reduction=*//* torch::kBatchMean */ at::Reduction::Mean);
  return loss;
}

// optional: L2 between predicted expected bone index? but we stick to KL

// ============= Training / Evaluation =============
void save_checkpoint(const SkinWeightGCN& model, const std::string& path) {
  torch::serialize::OutputArchive archive;
  model->save(archive);
  archive.save_to(path);
}

void load_checkpoint(SkinWeightGCN& model, const std::string& path) {
  torch::serialize::InputArchive archive;
  archive.load_from(path);
  model->load(archive);
}

FSys::path run_bone_weight_inference(const std::vector<FSys::path>& in_fbx_files, const FSys::path& in_output_path) {
  auto l_files = in_fbx_files;
  l_files |= ranges::actions::sort;
  return in_output_path;
}

int run(int argc, char** argv) {
  // basic CLI
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <data_dir_with_pt_samples>\n";
    return 1;
  }
  std::string data_dir = argv[1];

  // Gather .pt files in data_dir
  std::vector<std::string> files;
  for (auto& p : std::filesystem::directory_iterator(data_dir)) {
    if (p.path().extension() == ".pt") files.push_back(p.path().string());
  }
  if (files.empty()) {
    std::cerr << "No .pt sample files found in " << data_dir << "\n";
    return 1;
  }
  std::sort(files.begin(), files.end());

  // Split train/val (80/20)
  size_t n      = files.size();
  size_t ntrain = size_t(n * 0.8);
  std::vector<std::string> train_files(files.begin(), files.begin() + ntrain);
  std::vector<std::string> val_files(files.begin() + ntrain, files.end());

  GraphDataset train_ds(train_files);
  GraphDataset val_ds(val_files);

  // Model hyperparams
  int in_channels = 3 + 4;  // example: xyz + K distances (K=4)
  int hidden_dim  = 64;
  int num_bones   = 20;  // set according to dataset (can be changed or left 0 to require passing embeddings at forward)

  torch::Device device(torch::kCUDA);
  if (!torch::cuda::is_available()) {
    device = torch::Device(torch::kCPU);
    std::cout << "CUDA not available, using CPU\n";
  }

  SkinWeightGCN model{in_channels, hidden_dim, num_bones};
  model->to(device);

  torch::optim::Adam optimizer(model->parameters(), torch::optim::AdamOptions(1e-3));

  int epochs = 50;
  for (int epoch = 1; epoch <= epochs; ++epoch) {
    // --- training ---
    model->train();
    double epoch_loss = 0.0;
    for (size_t i = 0; i < train_files.size(); ++i) {
      auto sample = train_ds.load_sample_file(i);
      // move to device
      auto x      = sample.x.to(device);
      auto adj    = sample.adj.to(device);
      auto y      = sample.y.to(device);

      optimizer.zero_grad();
      // since we registered `bone_emb` in the model ctor, we can call forward without bone_embeddings
      auto logp = model->forward(x, adj);  // [N, B] log-probs
      auto loss = kl_loss_from_logprob(logp, y);
      loss.backward();
      optimizer.step();

      epoch_loss += loss.item<double>();
    }
    epoch_loss /= std::max<size_t>(1, train_files.size());

    // --- validation ---
    model->eval();
    double val_loss = 0.0;
    for (size_t i = 0; i < val_files.size(); ++i) {
      auto sample = val_ds.load_sample_file(i);
      auto x      = sample.x.to(device);
      auto adj    = sample.adj.to(device);
      auto y      = sample.y.to(device);
      torch::NoGradGuard no_grad;
      auto logp = model->forward(x, adj);
      auto loss = kl_loss_from_logprob(logp, y);
      val_loss += loss.item<double>();
    }
    val_loss /= std::max<size_t>(1, val_files.size());

    std::cout << "Epoch " << epoch << " TrainLoss: " << epoch_loss << " ValLoss: " << val_loss << "\n";

    // checkpoint
    if (epoch % 10 == 0) {
      std::string ckpt = "skin_gcn_epoch_" + std::to_string(epoch) + ".pt";
      save_checkpoint(model, ckpt);
      std::cout << "Saved checkpoint: " << ckpt << "\n";
    }
  }

  // final save
  save_checkpoint(model, "skin_gcn_final.pt");
  std::cout << "Training finished. Saved skin_gcn_final.pt\n";
  return 0;
}

}  // namespace doodle::ai