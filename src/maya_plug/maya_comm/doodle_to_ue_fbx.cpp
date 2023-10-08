//
// Created by td_main on 2023/9/26.
//

#include "doodle_to_ue_fbx.h"

#include <maya_plug/data/maya_tool.h>
#include <maya_plug/fmt/fmt_dag_path.h>

#include <fbxsdk.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagPathArray.h>
#include <maya/MEulerRotation.h>
#include <maya/MFloatArray.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MFnTransform.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItSelectionList.h>
#include <maya/MObjectArray.h>
#include <maya/MPointArray.h>
#include <maya/MQuaternion.h>
#include <maya/MSelectionList.h>
#include <treehh/tree.hh>

namespace doodle {
namespace maya_plug {

namespace {
constexpr char file_path[]   = "-file_path";
constexpr char file_path_l[] = "-fp";
}  // namespace

MSyntax doodle_to_ue_fbx_syntax() {
  MSyntax l_syntax{};
  l_syntax.addFlag(file_path, file_path_l, MSyntax::kString);
  l_syntax.setObjectType(MSyntax::kSelectionList);
  l_syntax.useSelectionAsDefault(true);
  return l_syntax;
}

// fbx 皮肤写入数据
struct fbx_skin_data {
  MObject skin{};
  MObjectArray joints{};
};

struct fbx_write_data {
  FbxLayerElementUV* mesh_2_uv(MFnMesh& in_mesh, MString& in_set_name) {
    auto* l_layer = FbxLayerElementUV::Create(mesh, in_set_name.asChar());
    l_layer->SetMappingMode(FbxLayerElement::eByPolygonVertex);
    l_layer->SetReferenceMode(FbxLayerElement::eIndexToDirect);
    // for maya uv
    MFloatArray l_u{};
    MFloatArray l_v{};
    in_mesh.getUVs(l_u, l_v, &in_set_name);
    for (auto i = 0; i < l_u.length(); ++i) {
      l_layer->GetDirectArray().Add(FbxVector2{l_u[i], l_v[i]});
    }

    auto l_face_count = in_mesh.numPolygons();
    for (auto i = 0; i < l_face_count; ++i) {
      MIntArray l_vert_list{};
      maya_chick(in_mesh.getPolygonVertices(i, l_vert_list));
      for (auto j = 0; j < l_vert_list.length(); ++j) {
        std::int32_t l_uv_id{};
        maya_chick(in_mesh.getPolygonUVid(i, j, l_uv_id, &in_set_name));
        l_layer->GetIndexArray().Add(l_uv_id);
      }
    }
    return l_layer;
  }

  FbxNode* node{};
  FbxMesh* mesh{};

  void write_mesh(MDagPath& in_mesh) {
    write_transform(in_mesh);
    if (!in_mesh.hasFn(MFn::kMesh)) {
      //      log_info(fmt::format("{} is not mesh", get_node_name(in_mesh)));
      return;
    }
    maya_chick(in_mesh.extendToShape());
    MFnMesh l_mesh{in_mesh};
    mesh = FbxMesh::Create(node->GetScene(), l_mesh.name().asChar());
    node->SetNodeAttribute(mesh);
    // 顶点
    {
      const auto l_point_count = l_mesh.numVertices();
      mesh->InitControlPoints(l_point_count);
      auto* l_points = mesh->GetControlPoints();
      MPointArray l_m_points{};
      l_mesh.getPoints(l_m_points, MSpace::kObject);
      for (auto i = 0; i < l_point_count; ++i) {
        l_points[i] = FbxVector4{l_m_points[i].x, l_m_points[i].y, l_m_points[i].z, l_m_points[i].w};
      }
    }
    // 三角形
    {
      MIntArray l_vert_list{};
      for (auto i = 0; i < l_mesh.numPolygons(); ++i) {
        mesh->BeginPolygon();
        maya_chick(l_mesh.getPolygonVertices(i, l_vert_list));
        for (auto j = 0; j < l_vert_list.length(); ++j) {
          mesh->AddPolygon(l_vert_list[j]);
        }
        mesh->EndPolygon();
      }
    }
    // uv
    auto l_main_layer = mesh->GetLayer(mesh->CreateLayer());
    {
      // get uv set names
      MStringArray l_uv_set_names{};
      maya_chick(l_mesh.getUVSetNames(l_uv_set_names));
      for (auto&& i_name : l_uv_set_names) {
        auto* l_layer = mesh_2_uv(l_mesh, i_name);
        l_main_layer->SetUVs(l_layer, FbxLayerElement::eTextureDiffuse);
        break;
      }
      if (l_uv_set_names.length() > 1) {
        log_error(fmt::format("mesh {} uv set length > 1", get_node_name(in_mesh)));

        for (auto i = 1; i < l_uv_set_names.length(); ++i) {
          auto* l_uv_layer = mesh->GetLayer(mesh->CreateLayer());
          auto* l_layer    = mesh_2_uv(l_mesh, l_uv_set_names[i]);
          l_uv_layer->SetUVs(l_layer, FbxLayerElement::eTextureDiffuse);
        }
      }
    }

    // normals
    {
      auto l_layer = FbxLayerElementNormal::Create(mesh, "");
      l_layer->SetMappingMode(FbxLayerElement::eByPolygonVertex);
      l_layer->SetReferenceMode(FbxLayerElement::eDirect);

      for (auto i = 0; i < l_mesh.numPolygons(); ++i) {
        MIntArray l_vert_list{};
        maya_chick(l_mesh.getPolygonVertices(i, l_vert_list));
        for (auto j = 0; j < l_vert_list.length(); ++j) {
          MVector l_normal{};
          maya_chick(l_mesh.getFaceVertexNormal(i, l_vert_list[j], l_normal, MSpace::kObject));
          l_layer->GetDirectArray().Add(FbxVector4{l_normal.x, l_normal.y, l_normal.z});
        }
      }

      l_main_layer->SetNormals(l_layer);
    }
    // smoothing
    {
      auto l_layer = FbxLayerElementSmoothing::Create(mesh, "");
      l_layer->SetMappingMode(FbxLayerElement::eByEdge);
      l_layer->SetReferenceMode(FbxLayerElement::eDirect);

      for (auto i = 0; i < l_mesh.numEdges(); ++i) {
        l_layer->GetDirectArray().Add(l_mesh.isEdgeSmooth(i));
      }
      l_main_layer->SetSmoothing(l_layer);
    }
  }

  void write_transform(const MDagPath& in_mesh) {
    MFnTransform l_transform{in_mesh};
    MStatus l_status{};
    auto l_loc = l_transform.getTranslation(MSpace::kTransform, &l_status);
    maya_chick(l_status);
    node->LclTranslation.Set({l_loc.x, l_loc.y, l_loc.z});
    MEulerRotation l_rot{};
    maya_chick(l_transform.getRotation(l_rot));

    node->LclRotation.Set(FbxVector4{l_rot.x, l_rot.y, l_rot.z});
    switch (l_rot.order) {
      case MTransformationMatrix::kXYZ:
        node->RotationOrder = FbxEuler::eOrderXYZ;
        break;
      case MTransformationMatrix::kYZX:
        node->RotationOrder = FbxEuler::eOrderYZX;
        break;
      case MTransformationMatrix::kZXY:
        node->RotationOrder = FbxEuler::eOrderZXY;
        break;
      case MTransformationMatrix::kXZY:
        node->RotationOrder = FbxEuler::eOrderXZY;
        break;
      case MTransformationMatrix::kYXZ:
        node->RotationOrder = FbxEuler::eOrderYXZ;
        break;
      case MTransformationMatrix::kZYX:
        node->RotationOrder = FbxEuler::eOrderZYX;
        break;

      default:
        node->RotationOrder = FbxEuler::eOrderXYZ;
        break;
    }
    std::double_t l_scale[3]{};
    l_transform.getScale(l_scale);
    node->LclScaling.Set({l_scale[0], l_scale[1], l_scale[2]});
  }

  void write_skeletion() {
    auto l_sk_attr = FbxSkeleton::Create(node->GetScene(), "skeleton");
    l_sk_attr->SetSkeletonType(FbxSkeleton::eLimbNode);
    node->SetNodeAttribute(l_sk_attr);
  }

  static std::vector<MDagPath> find_joint(const MObject& in_msk) {
    if (in_msk.isNull()) return {};
    MFnSkinCluster l_skin_cluster{in_msk};
    MDagPathArray l_joint_array{};
    MStatus l_status{};
    auto l_joint_count = l_skin_cluster.influenceObjects(l_joint_array, &l_status);
    maya_chick(l_status);

    std::vector<MDagPath> l_joint_vector{};
    for (auto i = 0; i < l_joint_count; ++i) {
      l_joint_vector.emplace_back(l_joint_array[i]);
    }
    return l_joint_vector;
  }
};

struct tree_dag_node {
  MDagPath dag_path{};
  FbxNode* node{};
  MObject skin_cluster_{};

  FbxNode* skin_cluster_fbx_{};

  std::function<void(tree_dag_node*)> write_file_{};

  void write() { write_file_(this); }
};

class doodle_to_ue_fbx::impl_data {
 public:
  using tree_mesh_t = tree<tree_dag_node>;

 private:
 public:
  std::shared_ptr<FbxManager> manager_{};
  FbxScene* scene_{};
  tree_mesh_t tree_dag_{};
  tree_mesh_t tree_bone_dag_{};

  std::vector<MDagPath> joints_{};

  void build_joint_tree() {
    for (auto&& i : joints_) {
      auto l_begin = tree_dag_.begin();
      for (std::int32_t j = i.length() - 1; j >= 0; --j) {
        MDagPath l_sub_path{i};
        l_sub_path.pop(j);

        if (auto l_tree_it = ranges::find_if(
                std::begin(l_begin), std::end(l_begin),
                [&](const impl_data::tree_mesh_t::value_type& in_value) { return in_value.dag_path == l_sub_path; }
            );
            l_tree_it != std::end(l_begin)) {
          l_begin = l_tree_it;
        } else {
          l_begin              = tree_dag_.append_child(l_begin, tree_dag_node{l_sub_path});
          l_begin->write_file_ = [](tree_dag_node* self) {
            fbx_write_data l_data{self->node, nullptr};
            l_data.write_transform(self->dag_path);
            if (!self->skin_cluster_.isNull() && self->skin_cluster_.hasFn(MFn::kJoint)) l_data.write_skeletion();
          };
        }
      }
    }
  }

  void build_mesh_tree(const MSelectionList& in_list) {
    MDagPath l_path{};
    for (MItSelectionList l_it{in_list}; !l_it.isDone(); l_it.next()) {
      maya_chick(l_it.getDagPath(l_path));
      // for dag path
      auto l_begin = tree_dag_.begin();
      for (std::int32_t i = l_path.length() - 1; i >= 0; --i) {
        MDagPath l_sub_path{l_path};
        l_sub_path.pop(i);

        if (auto l_tree_it = ranges::find_if(
                std::begin(l_begin), std::end(l_begin),
                [&](const impl_data::tree_mesh_t::value_type& in_value) { return in_value.dag_path == l_sub_path; }
            );
            l_tree_it != std::end(l_begin)) {
          l_begin = l_tree_it;
        } else {
          l_begin              = tree_dag_.append_child(l_begin, tree_dag_node{l_sub_path});
          l_begin->write_file_ = [](tree_dag_node* self) {
            fbx_write_data l_data{self->node, nullptr};
            l_data.write_mesh(self->dag_path);
          };
          l_begin->skin_cluster_ = get_skin_custer(l_begin->dag_path);
          if (!l_begin->skin_cluster_.isNull() && l_begin->skin_cluster_.hasFn(MFn::kSkinClusterFilter)) {
            MGlobal::displayInfo(conv::to_ms(fmt::format("写出皮肤簇 {}", l_begin->dag_path)));
            joints_ |= ranges::action::push_back(fbx_write_data::find_joint(l_begin->skin_cluster_));
          }
        }
      }
    }
  }

  void init() {
    tree_dag_ = {tree_dag_node{MDagPath{}, scene_->GetRootNode(), MObject::kNullObj, nullptr, [](auto...) {}}};
  }

  void build_tree(const MSelectionList& in_list) {
    build_mesh_tree(in_list);
    build_joint_tree();
    iter_tree(tree_dag_.begin());
  }

  void write_joint() {
    tree_bone_dag_ = {tree_dag_node{MDagPath{}, scene_->GetRootNode()}};
    build_joint_tree();
    iter_tree_2(tree_bone_dag_.begin());
  }

  MObject get_skin_custer(MDagPath in_dag_path) {
    if (!in_dag_path.hasFn(MFn::kMesh)) return {};

    MStatus l_s{};
    MObject l_skin_cluster{};
    maya_chick(in_dag_path.extendToShape());
    /// \brief 获得组件点上下文
    auto l_shape = in_dag_path.node(&l_s);
    maya_chick(l_s);

    /// 寻找高模的皮肤簇
    for (MItDependencyGraph i{l_shape, MFn::kSkinClusterFilter, MItDependencyGraph::Direction::kUpstream}; !i.isDone();
         i.next()) {
      l_skin_cluster = i.currentItem(&l_s);
      maya_chick(l_s);
    }
    return l_skin_cluster;
  }
};

doodle_to_ue_fbx::doodle_to_ue_fbx() : p_i{std::make_unique<impl_data>()} {}

MStatus doodle_to_ue_fbx::doIt(const MArgList& in_list) {
  MStatus l_statu{};
  MArgDatabase l_arg_data{syntax(), in_list, &l_statu};
  maya_chick(l_statu);
  MSelectionList l_list{};
  maya_chick(l_arg_data.getObjects(l_list));

  p_i->manager_ = std::shared_ptr<FbxManager>{FbxManager::Create(), [](FbxManager* in_ptr) { in_ptr->Destroy(); }};
  p_i->scene_   = FbxScene::Create(p_i->manager_.get(), "doodle_to_ue_fbx");
  p_i->manager_->SetIOSettings(FbxIOSettings::Create(p_i->manager_.get(), IOSROOT));

  auto l_doc_info      = FbxDocumentInfo::Create(p_i->manager_.get(), "DocInfo");
  l_doc_info->mTitle   = "doodle fbx";
  l_doc_info->mSubject = "doodle fbx";
  l_doc_info->mAuthor  = "doodle";
  l_doc_info->Original_ApplicationVendor.Set("doodle");
  l_doc_info->Original_ApplicationName.Set("doodle");
  l_doc_info->Original_ApplicationVersion.Set("1.0.0");

  l_doc_info->LastSaved_ApplicationVendor.Set("doodle");
  l_doc_info->LastSaved_ApplicationName.Set("doodle");
  l_doc_info->LastSaved_ApplicationVersion.Set("1.0.0");
  p_i->scene_->SetSceneInfo(l_doc_info);
  p_i->scene_->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::cm);
  auto anim_stack = FbxAnimStack::Create(p_i->scene_, "anim_stack");
  auto anim_layer = FbxAnimLayer::Create(p_i->scene_, "anim_layer");
  anim_stack->AddMember(anim_layer);

  p_i->init();
  p_i->build_tree(l_list);
  p_i->write_joint();
  write_fbx();
  return MS::kSuccess;
}

void doodle_to_ue_fbx::write_fbx() {
  auto* l_manager = p_i->scene_->GetFbxManager();
  std::shared_ptr<FbxExporter> l_exporter{
      FbxExporter::Create(p_i->scene_->GetFbxManager(), ""), [](FbxExporter* in_exporter) { in_exporter->Destroy(); }};
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_MATERIAL, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_TEXTURE, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_EMBEDDED, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_SHAPE, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_GOBO, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_ANIMATION, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_ASCIIFBX, true);

  if (!l_exporter->Initialize(
          //          in_fbx_path.string().c_str(),
          "D:/test.fbx", l_manager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX ascii (*.fbx)"),
          p_i->scene_->GetFbxManager()->GetIOSettings()
      )) {
    displayError(conv::to_ms(fmt::format("fbx exporter Initialize error: {}", l_exporter->GetStatus().GetErrorString()))
    );
  }
  l_exporter->Export(p_i->scene_);
}

doodle_to_ue_fbx::~doodle_to_ue_fbx() = default;

}  // namespace maya_plug
}  // namespace doodle