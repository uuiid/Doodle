#include "load_fbx.h"

#include <boost/numeric/conversion/cast.hpp>

#include <array>
#include <chrono>
#include <cmath>
#include <spdlog/spdlog.h>
#include <vector>

namespace doodle::ai {
namespace {
struct event_timer {
  using duration_type = std::chrono::microseconds;
  duration_type* duration_;
  std::chrono::high_resolution_clock::time_point start_time_;
  explicit event_timer(chrono::microseconds* duration)
      : duration_(duration), start_time_(std::chrono::high_resolution_clock::now()) {}
  ~event_timer() {
    auto end_time = std::chrono::high_resolution_clock::now();
    if (duration_) *duration_ += std::chrono::duration_cast<chrono::microseconds>(end_time - start_time_);
  }
  void stop() {
    auto end_time = std::chrono::high_resolution_clock::now();
    if (duration_) *duration_ += std::chrono::duration_cast<chrono::microseconds>(end_time - start_time_);
  }
};
}  // namespace

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

fbx_loader::fbx_loader(const FSys::path& in_fbx_path, logger_ptr_raw in_logger) {
  if (!in_logger) logger_ = spdlog::default_logger_raw();
  event_timer::duration_type l_load_duration{};
  event_timer timer(&l_load_duration);

  manager_ = std::shared_ptr<fbxsdk::FbxManager>(fbxsdk::FbxManager::Create(), [](fbxsdk::FbxManager* in_ptr) {
    in_ptr->Destroy();
  });
  fbxsdk::FbxIOSettings* ios = fbxsdk::FbxIOSettings::Create(manager_.get(), IOSROOT);
  manager_->SetIOSettings(ios);
  scene_                        = fbxsdk::FbxScene::Create(manager_.get(), "myScene");
  fbxsdk::FbxImporter* importer = fbxsdk::FbxImporter::Create(manager_.get(), "");
  if (!importer->Initialize(in_fbx_path.generic_string().c_str(), -1, manager_->GetIOSettings()))
    throw_exception(doodle_error{"fbx open err {}", importer->GetStatus().GetErrorString()});
  importer->Import(scene_);
  preprocessing();
  mesh_ = mesh_node_->GetMesh();
  timer.stop();
  SPDLOG_LOGGER_WARN(logger_, "加载fbx文件用时 {:%T}", l_load_duration);
}

void fbx_loader::preprocessing() {
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

fbx_load_result fbx_loader::load_fbx() {
  fbx_load_result l_result{};
  event_timer::duration_type l_load_duration{};
  event_timer timer(&l_load_duration);

  auto* l_vert       = mesh_->GetControlPoints();
  auto l_vert_count  = mesh_->GetControlPointsCount();
  l_result.vertices_ = torch::zeros({l_vert_count, 3}, torch::kFloat32);
  for (auto j = 0; j < l_vert_count; j++) {
    auto l_pos            = l_vert[j] + root_offset_;
    l_result.vertices_[j] = torch::tensor({l_pos[0], l_pos[1], l_pos[2]});
  }

  auto l_faces_num = mesh_->GetPolygonCount();
  l_result.faces_  = torch::zeros({l_faces_num, 3}, torch::kInt64);
  for (auto j = 0; j < l_faces_num; j++) {
    for (auto k = 0; k < mesh_->GetPolygonSize(j); k++) {
      auto l_vert_index     = mesh_->GetPolygonVertex(j, k);
      l_result.faces_[j][k] = l_vert_index;
    }
  }
  // 法线
  auto l_normals    = mesh_->GetElementNormal();
  l_result.normals_ = torch::zeros({l_vert_count, 3}, torch::kFloat32);
  if (l_normals) {
    auto l_normal_mapping_mode = l_normals->GetMappingMode();
    auto l_normal_ref_mode     = l_normals->GetReferenceMode();
    auto& l_normal_direct_arr  = l_normals->GetDirectArray();
    auto& l_normal_index_arr   = l_normals->GetIndexArray();
    FbxVector4 l_normal;
    if (l_normal_mapping_mode == FbxGeometryElement::eByControlPoint) {
      for (auto j = 0; j < l_vert_count; j++) {
        int l_normal_index = j;
        if (l_normal_ref_mode == FbxGeometryElement::eIndexToDirect) {
          l_normal_index = l_normal_index_arr.GetAt(j);
        }
        l_normal             = l_normal_direct_arr.GetAt(l_normal_index);
        l_result.normals_[j] = torch::tensor({l_normal[0], l_normal[1], l_normal[2]});
      }

    } else if (l_normal_mapping_mode == FbxGeometryElement::eByPolygonVertex) {
      // 每个面的每个顶点都有法线 这里我们取平均面的法线
      auto l_polygon_count = mesh_->GetPolygonCount();
      std::vector<std::vector<std::array<std::float_t, 3>>> l_normals_accum(l_vert_count);
      std::size_t l_index = 0;
      for (auto k = 0; k < l_polygon_count; k++) {
        for (auto m = 0; m < mesh_->GetPolygonSize(k); m++, l_index++) {
          auto l_vert_index  = mesh_->GetPolygonVertex(k, m);
          int l_normal_index = l_index;
          if (l_normal_ref_mode == FbxGeometryElement::eIndexToDirect) {
            l_normal_index = l_normal_index_arr.GetAt(l_normal_index);
          }
          l_normal = l_normal_direct_arr.GetAt(l_normal_index);
          l_normals_accum[static_cast<std::size_t>(l_vert_index)].push_back(
              {boost::numeric_cast<std::float_t>(l_normal[0]), boost::numeric_cast<std::float_t>(l_normal[1]),
               boost::numeric_cast<std::float_t>(l_normal[2])}
          );
        }
      }
      for (auto j = 0; j < l_vert_count; j++) {
        auto& l_normal_list = l_normals_accum[static_cast<std::size_t>(j)];
        std::array<std::double_t, 3> l_normal_sum{0.0, 0.0, 0.0};
        for (auto& n : l_normal_list) {
          l_normal_sum[0] += n[0];
          l_normal_sum[1] += n[1];
          l_normal_sum[2] += n[2];
        }
        auto l_count = static_cast<std::double_t>(l_normal_list.size());
        if (l_count > 0) {
          l_result.normals_[j] = torch::tensor(
              {boost::numeric_cast<std::float_t>(l_normal_sum[0] / l_count),
               boost::numeric_cast<std::float_t>(l_normal_sum[1] / l_count),
               boost::numeric_cast<std::float_t>(l_normal_sum[2] / l_count)}
          );
        }
      }
    }
  }

  auto* l_sk = static_cast<FbxSkin*>(mesh_->GetDeformer(0, FbxDeformer::eSkin));
  if (!l_sk) throw_exception(doodle_error{"no skin found"});
  auto l_sk_count          = l_sk->GetClusterCount();
  l_result.bone_positions_ = torch::zeros({l_sk_count, 3}, torch::kFloat32);
  l_result.bone_weights_   = torch::zeros({l_vert_count, l_sk_count}, torch::kFloat32);
  std::map<FbxNode*, std::int64_t> l_bone_index_map{};
  for (auto i = 0; i < l_sk_count; i++) {
    auto l_cluster            = l_sk->GetCluster(i);
    auto l_joint              = l_cluster->GetLink();
    // logger_->warn("bone {} index {}", l_joint->GetName(), i);
    l_bone_index_map[l_joint] = i;
  }

  l_result.bone_parents_ = torch::full({l_sk_count}, -1, torch::kInt64);
  for (auto i = 0; i < l_sk_count; i++) {
    auto l_cluster = l_sk->GetCluster(i);
    auto l_joint   = l_cluster->GetLink();
    auto l_parent  = l_joint->GetParent();
    do {
      if (l_parent && l_bone_index_map.contains(l_parent)) {
        l_result.bone_parents_[i] = l_bone_index_map[l_parent];
        break;
      }
      l_parent = l_parent->GetParent();
    } while (l_parent);

    auto l_matrix               = scene_->GetAnimationEvaluator()->GetNodeGlobalTransform(l_joint);
    // FbxAMatrix l_matrix_tmp{};
    // l_cluster->GetTransformLinkMatrix(l_matrix_tmp);
    // l_matrix            = l_matrix * l_matrix_tmp;
    l_result.bone_positions_[i] = torch::tensor({l_matrix.GetT()[0], l_matrix.GetT()[1], l_matrix.GetT()[2]});
    auto l_controls             = l_cluster->GetControlPointIndices();
    auto l_weights              = l_cluster->GetControlPointWeights();
    for (auto j = 0; j < l_cluster->GetControlPointIndicesCount(); j++) {
      l_result.bone_weights_[l_controls[j]][i] = l_weights[j];
    }
  }
  // logger_->warn(
  //     "tensor \n{}\n bone_positions \n{}\n faces \n{}\n bone_weights \n{}\n bone_parents \n{}\n" l_tensor,
  //     l_bone_positions, l_faces, l_bone_weights, l_bone_parents
  // );

  timer.stop();

  SPDLOG_LOGGER_WARN(logger_, "读取fbx文件用时 {:%T}", l_load_duration);
  return l_result;
}

void fbx_loader::write_weights_to_fbx(const torch::Tensor& weights, const std::filesystem::path& out_path) {
  auto l_vert_count = mesh_->GetControlPointsCount();
  if (weights.size(0) != l_vert_count)
    throw_exception(doodle_error{"weights size mismatch: expected {}, got {}", l_vert_count, weights.size(0)});

  // Clear all skins to avoid ghost weights
  auto l_skin_count = mesh_->GetDeformerCount(FbxDeformer::eSkin);
  for (int s = 0; s < l_skin_count; ++s) {
    auto* l_skin_deformer = static_cast<FbxSkin*>(mesh_->GetDeformer(s, FbxDeformer::eSkin));
    auto l_cluster_count  = l_skin_deformer->GetClusterCount();
    for (int i = 0; i < l_cluster_count; ++i) {
      l_skin_deformer->GetCluster(i)->SetControlPointIWCount(0);
    }
  }

  auto* l_sk = static_cast<FbxSkin*>(mesh_->GetDeformer(0, FbxDeformer::eSkin));
  if (!l_sk) throw_exception(doodle_error{"no skin found"});
  auto l_sk_count  = l_sk->GetClusterCount();
  auto weights_acc = weights.accessor<float, 2>();
  // 打印骨骼权重
  // SPDLOG_WARN("writing weights to fbx: {}", weights);

  for (auto i = 0; i < l_sk_count; i++) {
    auto l_cluster = l_sk->GetCluster(i);
    for (auto j = 0; j < l_vert_count; j++) {
      auto w = weights_acc[j][i];
      if (w > 1e-6) l_cluster->AddControlPointIndex(j, w);
    }
  }
  write_fbx(out_path);
}

void fbx_loader::write_fbx(const FSys::path& out_path) {
  SPDLOG_WARN("writing fbx to {}", out_path.generic_string());
  auto exporter = std::shared_ptr<fbxsdk::FbxExporter>{
      fbxsdk::FbxExporter::Create(manager_.get(), ""), [](fbxsdk::FbxExporter* in_ptr) { in_ptr->Destroy(); }
  };
  if (!exporter->Initialize(
          out_path.generic_string().c_str(),
          manager_->GetIOPluginRegistry()->FindWriterIDByDescription("FBX ascii (*.fbx)"), manager_->GetIOSettings()
      ))
    throw_exception(doodle_error{"fbx export err {}", exporter->GetStatus().GetErrorString()});
  exporter->Export(scene_);
}

}  // namespace doodle::ai