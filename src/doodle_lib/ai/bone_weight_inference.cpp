#include "bone_weight_inference.h"

#include <mimalloc.h>

#include "doodle_core/core/file_sys.h"
#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/exception/exception.h>

#include <ATen/core/Reduction.h>
#include <c10/core/DeviceType.h>
#include <fbxsdk.h>
#include <fbxsdk/core/base/fbxarray.h>
#include <fbxsdk/core/fbxmanager.h>
#include <fbxsdk/core/math/fbxvector4.h>
#include <fbxsdk/scene/geometry/fbxnode.h>
#include <fbxsdk/scene/geometry/fbxskin.h>
#include <fbxsdk/scene/geometry/fbxtrimnurbssurface.h>
#include <fbxsdk/utils/fbxgeometryconverter.h>
#include <filesystem>
#include <fmt/format.h>
#include <functional>
#include <map>
#include <memory>
#include <range/v3/action/sort.hpp>
#include <sstream>
#include <torch/csrc/autograd/generated/variable_factories.h>
#include <torch/types.h>
#include <vector>

#pragma comment(linker, "/include:mi_version")

#include <torch/csrc/api/include/torch/torch.h>

namespace fmt {
template <typename Char_T>
struct formatter<::torch::Tensor, Char_T> : basic_ostream_formatter<Char_T> {};
}  // namespace fmt

namespace doodle::ai {

struct GraphSample {
  // node features: [num_nodes, in_dim]
  torch::Tensor x;
  // normalized adjacency: [num_nodes, num_nodes] (float)
  // we will use dense adjacency for simplicity
  torch::Tensor adj;
  // label distribution per node: [num_nodes, num_bones]
  torch::Tensor y;
  // optional: bone adjacency [B, B] (normalized)
  torch::Tensor bone_adj;
  // optional: node mask or other metadata
  // raw bone features (e.g. bone positions or other per-bone descriptors): [B, F]
  torch::Tensor bone_feat;
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
    const torch::Tensor& vertices,        // [N,3]
    const torch::Tensor& bone_positions,  // [B,3]
    const torch::Tensor& faces,           // [F,3] or empty
    const torch::Tensor& weights,         // [N,B] ground truth distribution
    int K /*=4*/, const std::vector<std::int64_t>& bone_parents = std::vector<std::int64_t>()
) {
  std::int64_t N  = vertices.size(0);
  std::int64_t B  = bone_positions.size(0);

  // 1) 面邻接（无向）
  auto options    = torch::TensorOptions().dtype(torch::kFloat32);
  torch::Tensor A = torch::zeros({N, N}, options);
  if (faces.numel() <= 0) throw std::runtime_error("faces tensor has negative numel");

  auto f_accessor = faces.accessor<std::int64_t, 2>();
  for (std::int64_t i = 0; i < faces.size(0); ++i) {
    std::int64_t a = f_accessor[i][0];
    std::int64_t b = f_accessor[i][1];
    std::int64_t c = f_accessor[i][2];
    A[a][b]        = 1;
    A[b][a]        = 1;
    A[b][c]        = 1;
    A[c][b]        = 1;
    A[c][a]        = 1;
    A[a][c]        = 1;
  }

  // 2) feature design: [vx, vy, vz] + vectors to K nearest bones
  // We use K = min(4, B) by default; for each vertex we find K nearest bones and
  // compute bone_vector = bone_position - vertex (shape [3]). We flatten K vectors
  // into a [K*3] block per vertex and concat with xyz to form features.
  // Clamp K to number of bones available
  // 2）
  // 特征设计：[vx，vy，vz]+向量到K个最近的骨骼我们默认使用K=min（4，B）；对于每个顶点，
  // 我们找到K个最近的骨骼，并计算bone_vector=bone_position顶点（shape[3]）。
  // 我们将K个向量展平为每个顶点[K*3]个块，并用xyz进行concat以形成特征。将K夹紧到可用的骨骼数量
  K                   = std::min<int>(K, (int)B);
  // expand for vectorized distance computation
  torch::Tensor v_exp = vertices.unsqueeze(1).expand({N, B, 3});        // [N,B,3]
  torch::Tensor b_exp = bone_positions.unsqueeze(0).expand({N, B, 3});  // [N,B,3]
  torch::Tensor dists = torch::norm(v_exp - b_exp, 2, -1);              // [N,B]
  // get K nearest bones per-vertex. Use topk instead of full sort to avoid
  // potential comparator issues in large sorts and for performance.
  // guard against NaNs/InFs in distances which may lead to unstable ordering
  // 获取每个顶点最近的K个骨骼。使用topk而不是全排序，以避免大排序中潜在的比较器问题，并提高性能。在距离上防止NaNs/InFs，这可能会导致排序不稳定
  if (torch::isnan(dists).any().item<bool>() || torch::isinf(dists).any().item<bool>()) {
    // replace NaN/Inf with a large finite value so they are treated as far-away
    auto large = torch::full_like(dists, 1e30f);
    dists      = torch::where(torch::isfinite(dists), dists, large);
  }

  // topk: values and indices of K smallest distances along dim=1
  // topk：沿dim=1的K最小距离的值和索引
  auto topk_res              = dists.topk(K, /*dim=*/1, /*largest=*/false, /*sorted=*/true);
  auto sorted_vals           = std::get<0>(topk_res);  // [N,K]
  auto sorted_idx            = std::get<1>(topk_res);  // [N,K]

  // prepare closest vectors tensor [N, K*3]
  auto vec_options           = torch::TensorOptions().dtype(torch::kFloat32);
  torch::Tensor closest_vecs = torch::zeros({N, K * 3}, vec_options);

  // Use accessors to avoid per-element tensor allocation inside loop
  auto sorted_idx_acc        = sorted_idx.accessor<std::int64_t, 2>();
  auto vert_acc              = vertices.accessor<float, 2>();
  auto bone_acc              = bone_positions.accessor<float, 2>();
  for (std::int64_t i = 0; i < N; ++i) {
    for (int j = 0; j < K; ++j) {
      std::int64_t bidx          = sorted_idx_acc[i][j];
      // compute bone_position - vertex
      float dx                   = bone_acc[bidx][0] - vert_acc[i][0];
      float dy                   = bone_acc[bidx][1] - vert_acc[i][1];
      float dz                   = bone_acc[bidx][2] - vert_acc[i][2];
      // write into closest_vecs (use tensor indexing of a single element)
      closest_vecs[i][j * 3 + 0] = dx;
      closest_vecs[i][j * 3 + 1] = dy;
      closest_vecs[i][j * 3 + 2] = dz;
    }
  }

  // feature concat: xyz + flattened K vectors -> [N, 3 + K*3]
  torch::Tensor x        = torch::cat({vertices, closest_vecs}, /*dim=*/1);  // [N, 3+K*3]

  // 3) normalize adjacency
  torch::Tensor adj_norm = normalize_adjacency(A);

  // 4) build bone adjacency
  // Prefer parent-child topology if provided (bone_parents length == B)
  torch::Tensor bone_adj = torch::zeros({B, B}, torch::TensorOptions().dtype(torch::kFloat32));
  if ((int)bone_parents.size() != B) throw std::runtime_error("bone_parents length != B");
  for (std::int64_t i = 0; i < B; ++i) {
    std::int64_t p = bone_parents[i];
    if (p >= 0 && p < B) {
      bone_adj[i][p] = 1.0;
      bone_adj[p][i] = 1.0;
    }
  }
  bone_adj = normalize_adjacency(bone_adj);

  GraphSample s;
  s.x         = x;
  s.adj       = adj_norm;
  s.bone_adj  = bone_adj;
  s.y         = weights;
  // include raw bone features in the sample so dataset/model can project them
  s.bone_feat = bone_positions;  // [B, 3]

  return s;
}

struct fbx_scene {
  std::shared_ptr<fbxsdk::FbxManager> manager_;
  fbxsdk::FbxScene* scene_;
  FSys::path fbx_path_;
  FbxNode* mesh_node_{};
  fbxsdk::FbxMesh* mesh_{};
  fbxsdk::FbxVector4 root_offset_{};

  logger_ptr_raw logger_;
  explicit fbx_scene(const std::filesystem::path& fbx_path, logger_ptr_raw in_logger)
      : fbx_path_(fbx_path), logger_(in_logger) {
    if (!in_logger) logger_ = spdlog::default_logger_raw();
    manager_ = std::shared_ptr<fbxsdk::FbxManager>(fbxsdk::FbxManager::Create(), [](fbxsdk::FbxManager* in_ptr) {
      in_ptr->Destroy();
    });
    fbxsdk::FbxIOSettings* ios = fbxsdk::FbxIOSettings::Create(manager_.get(), IOSROOT);
    manager_->SetIOSettings(ios);
    scene_                        = fbxsdk::FbxScene::Create(manager_.get(), "myScene");
    fbxsdk::FbxImporter* importer = fbxsdk::FbxImporter::Create(manager_.get(), "");
    if (!importer->Initialize(fbx_path.generic_string().c_str(), -1, manager_->GetIOSettings()))
      throw_exception(doodle_error{"fbx open err {}", importer->GetStatus().GetErrorString()});
    importer->Import(scene_);
    preprocessing();
    mesh_ = mesh_node_->GetMesh();
  }
  void preprocessing() {
    auto l_root = scene_->GetRootNode();
    FbxGeometryConverter l_converter{manager_.get()};
    l_converter.RecenterSceneToWorldCenter(scene_, 0.000001);
    root_offset_ = l_root->GetChild(0)->LclTranslation.Get();
    FbxArray<FbxNode*> l_mesh_nodes;

    std::function<void(FbxNode*)> l_fun;
    l_fun = [&](FbxNode* in_node) {
      for (auto i = 0; i < in_node->GetChildCount(); i++) {
        auto l_child = in_node->GetChild(i);
        if (l_child->GetNodeAttribute() && l_child->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh) {
          l_mesh_nodes.Add(l_child);
        } else {
          l_fun(l_child);
        }
      }
    };

    l_fun(l_root);
    if (l_mesh_nodes.Size() == 0) throw_exception(doodle_error{"fbx mesh not found"});

    mesh_node_ = l_converter.MergeMeshes(l_mesh_nodes, fmt::format("main_{}", l_mesh_nodes.Size()).c_str(), scene_);
    scene_->GetRootNode()->AddChild(mesh_node_);
    if (!mesh_node_) throw_exception(doodle_error{"merge mesh err"});
    for (auto i = 0; i < l_mesh_nodes.Size(); i++) scene_->RemoveNode(l_mesh_nodes[i]);
    l_converter.Triangulate(mesh_node_->GetMesh(), true);
  }

  GraphSample get_sample() {
    auto* l_vert           = mesh_->GetControlPoints();
    auto l_vert_count      = mesh_->GetControlPointsCount();
    torch::Tensor l_tensor = torch::zeros({l_vert_count, 3}, torch::kFloat32);
    for (auto j = 0; j < l_vert_count; j++) {
      auto l_pos  = l_vert[j] + root_offset_;
      l_tensor[j] = torch::tensor({l_pos[0], l_pos[1], l_pos[2]});
    }

    auto l_faces_num      = mesh_->GetPolygonCount();
    torch::Tensor l_faces = torch::zeros({l_faces_num, 3}, torch::kInt64);
    for (auto j = 0; j < l_faces_num; j++) {
      for (auto k = 0; k < mesh_->GetPolygonSize(j); k++) {
        auto l_vert_index = mesh_->GetPolygonVertex(j, k);
        l_faces[j][k]     = l_vert_index;
      }
    }
    auto* l_sk = static_cast<FbxSkin*>(mesh_->GetDeformer(0, FbxDeformer::eSkin));
    if (!l_sk) throw_exception(doodle_error{"no skin found"});
    auto l_sk_count                = l_sk->GetClusterCount();
    torch::Tensor l_bone_positions = torch::zeros({l_sk_count, 3}, torch::kFloat32);
    torch::Tensor l_bone_weights   = torch::zeros({l_vert_count, l_sk_count}, torch::kFloat32);
    std::map<FbxNode*, std::int64_t> l_bone_index_map{};
    for (auto i = 0; i < l_sk_count; i++) {
      auto l_cluster = l_sk->GetCluster(i);
      auto l_joint   = l_cluster->GetLink();
      logger_->warn("bone {} index {}", l_joint->GetName(), i);
      l_bone_index_map[l_joint] = i;
    }
    std::vector<std::int64_t> l_bone_parents(l_sk_count, -1);
    for (auto i = 0; i < l_sk_count; i++) {
      auto l_cluster = l_sk->GetCluster(i);
      auto l_joint   = l_cluster->GetLink();

      if (auto l_parent = l_joint->GetParent(); l_parent && l_bone_index_map.contains(l_parent)) {
        l_bone_parents[i] = l_bone_index_map[l_parent];
      }

      auto l_matrix       = scene_->GetAnimationEvaluator()->GetNodeGlobalTransform(l_joint);
      // FbxAMatrix l_matrix_tmp{};
      // l_cluster->GetTransformLinkMatrix(l_matrix_tmp);
      // l_matrix            = l_matrix * l_matrix_tmp;
      l_bone_positions[i] = torch::tensor({l_matrix.GetT()[0], l_matrix.GetT()[1], l_matrix.GetT()[2]});
      auto l_controls     = l_cluster->GetControlPointIndices();
      auto l_weights      = l_cluster->GetControlPointWeights();
      for (auto j = 0; j < l_cluster->GetControlPointIndicesCount(); j++) {
        l_bone_weights[l_controls[j]][i] = l_weights[j];
      }
    }
    // logger_->warn(
    //     "tensor \n{}\n bone_positions \n{}\n faces \n{}\n bone_weights \n{}\n bone_parents \n{}\n" l_tensor,
    //     l_bone_positions, l_faces, l_bone_weights, l_bone_parents
    // );
    return build_sample_from_mesh(l_tensor, l_bone_positions, l_faces, l_bone_weights, 4, l_bone_parents);
  }
  void write_weights_to_fbx(const torch::Tensor& weights) {
    auto l_vert_count = mesh_->GetControlPointsCount();
    if (weights.size(0) != l_vert_count)
      throw_exception(doodle_error{"weights size mismatch: expected {}, got {}", l_vert_count, weights.size(0)});
    auto* l_sk = static_cast<FbxSkin*>(mesh_->GetDeformer(0, FbxDeformer::eSkin));
    if (!l_sk) throw_exception(doodle_error{"no skin found"});
    auto l_sk_count  = l_sk->GetClusterCount();
    auto weights_acc = weights.accessor<float, 2>();
    for (auto i = 0; i < l_sk_count; i++) {
      auto l_cluster = l_sk->GetCluster(i);
      l_cluster->SetControlPointIWCount(0);
      for (auto j = 0; j < l_vert_count; j++) {
        auto w = weights_acc[j][i];
        if (w > 1e-6) l_cluster->AddControlPointIndex(j, w);
      }
    }
    write_fbx(fbx_path_);
  }

  void write_fbx(const std::filesystem::path& out_path) {
    fbxsdk::FbxExporter* exporter = fbxsdk::FbxExporter::Create(manager_.get(), "");
    if (!exporter->Initialize(
            out_path.generic_string().c_str(),
            manager_->GetIOPluginRegistry()->FindWriterIDByDescription("FBX ascii (*.fbx)"), manager_->GetIOSettings()
        ))
      throw_exception(doodle_error{"fbx export err {}", exporter->GetStatus().GetErrorString()});
    exporter->Export(scene_);
    exporter->Destroy();
  }
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
  // This model now expects raw per-bone features (bone_feat) of dimension F.
  // The model contains a learnable projector `bone_proj` which maps bone_feat -> hidden_dim.
  // bone_proj makes the dataset free to provide raw bone descriptors (e.g. positions, transforms).
  SkinWeightGCNImpl(int in_channels, int hidden_dim, int bone_feat_dim = 3)
      : gc1(nullptr), gc2(nullptr), bgc(nullptr), bone_proj(nullptr), gfc1(nullptr), gfc2(nullptr), fc1(nullptr) {
    gc1       = register_module("gc1", GraphConv(in_channels, hidden_dim));
    gc2       = register_module("gc2", GraphConv(hidden_dim, hidden_dim));
    // bone graph conv: transforms bone embeddings via bone adjacency
    bgc       = register_module("bgc", GraphConv(hidden_dim, hidden_dim));
    // linear projector to map raw bone features [B, bone_feat_dim] -> [B, hidden_dim]
    bone_proj = register_module("bone_proj", torch::nn::Linear(bone_feat_dim, hidden_dim));
    // global branch
    gfc1      = register_module("gfc1", torch::nn::Linear(in_channels, hidden_dim));
    gfc2      = register_module("gfc2", torch::nn::Linear(hidden_dim, hidden_dim));
    // fusion and final
    fc1       = register_module("fc1", torch::nn::Linear(hidden_dim * 2, hidden_dim));
  }

  // forward returns log probabilities per node (for KLDiv use)
  // If `bone_embeddings` is provided it will be used (shape [B, hidden_dim]).
  // Otherwise the registered `bone_emb` (if any) will be used.
  // forward now accepts raw `bone_feat` of shape [B, F] (F == bone_feat_dim passed to ctor).
  torch::Tensor forward(
      const torch::Tensor& x, const torch::Tensor& adj_norm, const torch::Tensor& bone_feat,
      const torch::Tensor& bone_adj
  ) {
    // local branch
    auto l       = torch::relu(gc1->forward(x, adj_norm));  // [N, H]
    l            = torch::relu(gc2->forward(l, adj_norm));  // [N, H]

    // global branch: linear on node features -> pooled
    auto g       = torch::relu(gfc1->forward(x));  // [N, H]
    g            = torch::relu(gfc2->forward(g));  // [N, H]
    auto g_pool  = g.mean(0, /*keepdim=*/true);    // [1, H]
    // broadcast to nodes
    auto g_bcast = g_pool.expand({x.size(0), g_pool.size(1)});  // [N,H]

    // concat
    auto feat    = torch::cat({l, g_bcast}, /*dim=*/1);  // [N, 2H]
    auto h       = torch::relu(fc1->forward(feat));      // [N, H]

    // project raw bone features to hidden space
    if (!bone_feat.defined()) throw std::runtime_error("bone_feat must be provided to the model forward");
    if (bone_feat.dim() != 2) throw std::runtime_error("bone_feat must be a 2D tensor [B, F]");
    // project: [B, F] -> [B, H]
    auto used_emb = torch::relu(bone_proj->forward(bone_feat));

    // validate bone_adj and used_emb shapes/devices
    if (!bone_adj.defined()) throw std::runtime_error("bone_adj must be provided to aggregate bone embeddings");
    if (bone_adj.dim() != 2) throw std::runtime_error("bone_adj must be a 2D adjacency matrix [B, B]");
    if (bone_adj.size(0) != bone_adj.size(1) || bone_adj.size(0) != used_emb.size(0)) {
      std::ostringstream ss;
      ss << "bone_adj must be square [B,B] and match used_emb rows: bone_adj=" << bone_adj.sizes()
         << " used_emb=" << used_emb.sizes();
      throw std::runtime_error(ss.str());
    }
    if (bone_adj.device() != used_emb.device()) {
      std::ostringstream ss;
      ss << "bone_adj device (" << bone_adj.device() << ") does not match bone embeddings device (" << used_emb.device()
         << ")";
      throw std::runtime_error(ss.str());
    }
    // aggregate via bone graph conv
    used_emb = bgc->forward(used_emb, bone_adj);

    // used_emb: [B, H]
    // shape checks: used_emb must be 2D and its second dim must match h.size(1)
    if (used_emb.dim() != 2) {
      throw std::runtime_error("bone_embeddings must be a 2D tensor of shape [B, hidden_dim]");
    }
    auto emb_hidden_dim  = used_emb.size(1);
    auto node_hidden_dim = h.size(1);
    if (emb_hidden_dim != node_hidden_dim) {
      std::ostringstream ss;
      ss << "bone_embeddings hidden-dim (" << emb_hidden_dim << ") does not match model hidden-dim (" << node_hidden_dim
         << ").";
      throw std::runtime_error(ss.str());
    }
    auto logits = torch::matmul(h, used_emb.t());  // [N, B]
    return torch::log_softmax(logits, /*dim=*/1);
  }

  GraphConv gc1, gc2;
  GraphConv bgc;
  torch::nn::Linear bone_proj;
  torch::nn::Linear gfc1, gfc2, fc1;
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

std::shared_ptr<bone_weight_inference_model> bone_weight_inference_model::train(
    const std::vector<FSys::path>& in_fbx_files, const FSys::path& in_output_path
) {
  auto l_files = in_fbx_files;
  l_files |= ranges::actions::sort;
  size_t n      = l_files.size();
  size_t ntrain = size_t(n * 0.8);
  std::vector<FSys::path> train_files(l_files.begin(), l_files.begin() + ntrain);
  std::vector<FSys::path> val_files(l_files.begin() + ntrain, l_files.end());

  // Model hyperparams
  int K           = 4;          // number of nearest bone vectors used per vertex
  int in_channels = 3 + K * 3;  // example: xyz + K * (dx,dy,dz)
  int hidden_dim  = 64;

  torch::Device device(torch::kCUDA);
  if (!torch::cuda::is_available()) {
    device = torch::Device(torch::kCPU);
    std::cout << "CUDA not available, using CPU\n";
  }

  SkinWeightGCN model{in_channels, hidden_dim};
  model->to(device);

  torch::optim::Adam optimizer(model->parameters(), torch::optim::AdamOptions(1e-3));

  int epochs = 50;
  for (int epoch = 1; epoch <= epochs; ++epoch) {
    // --- training ---
    model->train();
    double epoch_loss = 0.0;
    for (size_t i = 0; i < train_files.size(); ++i) {
      fbx_scene l_fbx{train_files[i], nullptr};
      auto sample = l_fbx.get_sample();
      // move to device
      auto x      = sample.x.to(device);
      auto adj    = sample.adj.to(device);
      auto y      = sample.y.to(device);

      optimizer.zero_grad();
      // pass optional bone_adj (may be undefined)
      torch::Tensor bone_adj  = sample.bone_adj.to(device);
      torch::Tensor bone_feat = sample.bone_feat.to(device);
      auto logp               = model->forward(x, adj, bone_feat, bone_adj);  // [N, B] log-probs
      auto loss               = kl_loss_from_logprob(logp, y);
      loss.backward();
      optimizer.step();

      epoch_loss += loss.item<double>();
    }
    epoch_loss /= std::max<size_t>(1, train_files.size());

    // --- validation ---
    model->eval();
    double val_loss = 0.0;
    for (size_t i = 0; i < val_files.size(); ++i) {
      fbx_scene l_fbx{val_files[i], nullptr};
      auto sample = l_fbx.get_sample();
      auto x      = sample.x.to(device);
      auto adj    = sample.adj.to(device);
      auto y      = sample.y.to(device);
      torch::NoGradGuard no_grad;
      torch::Tensor bone_adj  = sample.bone_adj.to(device);
      torch::Tensor bone_feat = sample.bone_feat.to(device);
      auto logp               = model->forward(x, adj, bone_feat, bone_adj);
      auto loss               = kl_loss_from_logprob(logp, y);
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
  save_checkpoint(model, in_output_path.generic_string());
  return {};
}
class bone_weight_inference_model::impl {
 public:
  torch::Device device_;
  SkinWeightGCN model_;

 public:
  explicit impl(const FSys::path& in_model_path) : device_(torch::kCUDA), model_(15, 64) {
    if (!torch::cuda::is_available()) {
      device_ = torch::Device(torch::kCPU);
      std::cout << "CUDA not available, using CPU\n";
    }
    model_->to(device_);
    load_checkpoint(model_, in_model_path.generic_string());
    model_->eval();
  }
};
void bone_weight_inference_model::load_model(const FSys::path& in_model_path) {
  pimpl_ = std::make_shared<impl>(in_model_path);
}

void bone_weight_inference_model::predict_by_fbx(
    const FSys::path& in_fbx_path, const FSys::path& out_fbx_path, logger_ptr_raw in_logger
) {
  if (!pimpl_) throw_exception(doodle_error{"模型未加载"});
  fbx_scene l_fbx{in_fbx_path, in_logger};
  auto sample             = l_fbx.get_sample();
  auto x                  = sample.x.to(pimpl_->device_);
  auto adj                = sample.adj.to(pimpl_->device_);
  torch::Tensor bone_adj  = sample.bone_adj.to(pimpl_->device_);
  torch::Tensor bone_feat = sample.bone_feat.to(pimpl_->device_);
  torch::Tensor logp      = pimpl_->model_->forward(x, adj, bone_feat, bone_adj);
  auto bone_weights       = torch::exp(logp);
  l_fbx.write_weights_to_fbx(bone_weights.cpu());
}

}  // namespace doodle::ai