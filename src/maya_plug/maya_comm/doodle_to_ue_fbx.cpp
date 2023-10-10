//
// Created by td_main on 2023/9/26.
//

#include "doodle_to_ue_fbx.h"

#include <boost/lambda2.hpp>

#include <maya_plug/data/dagpath_cmp.h>
#include <maya_plug/data/maya_tool.h>
#include <maya_plug/fmt/fmt_dag_path.h>
#include <maya_plug/fmt/fmt_warp.h>

#include <fbxsdk.h>
#include <maya/MAngle.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagPathArray.h>
#include <maya/MEulerRotation.h>
#include <maya/MFloatArray.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MFnTransform.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItGeometry.h>
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

struct tree_dag_node /* : boost::equality_comparable<tree_dag_node> */ {
  MDagPath dag_path{};
  FbxNode* node{};
  MObject skin_cluster_{};

  std::function<void(tree_dag_node*)> write_file_{};
  std::shared_ptr<std::once_flag> write_flag_{std::make_shared<std::once_flag>()};
  void write() {
    std::call_once(*write_flag_, write_file_, this);
    //    write_file_(this);
  }

  //  bool operator!=(const tree_dag_node& rhs) const {
  //    auto l_string  = dag_path.fullPathName();
  //    auto l_string2 = rhs.dag_path.fullPathName();
  //    return std::tie(l_string, node) != std::tie(l_string2, rhs.node);
  //  }
};
using tree_mesh_t = tree<tree_dag_node>;

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
    write_loc_attr(in_mesh);
    auto l_attr_null = FbxNull::Create(node->GetScene(), l_transform.name().asChar());
    l_attr_null->Look.Set(FbxNull::eNone);
    node->SetNodeAttribute(l_attr_null);

    //    node->SetRotationOrder(FbxNode::eSourcePivot, node->RotationOrder.Get());
  }

  void write_loc_attr(const MDagPath& in_mesh) {
    MFnTransform l_transform{in_mesh};
    MStatus l_status{};
    node->SetRotationActive(true);
    MEulerRotation l_rot{};
    maya_chick(l_transform.getRotation(l_rot));
    switch (l_rot.order) {
      case MTransformationMatrix::kXYZ:
        node->RotationOrder.Set(FbxEuler::eOrderXYZ);
        break;
      case MTransformationMatrix::kYZX:
        node->RotationOrder.Set(FbxEuler::eOrderYZX);
        break;
      case MTransformationMatrix::kZXY:
        node->RotationOrder.Set(FbxEuler::eOrderZXY);
        break;
      case MTransformationMatrix::kXZY:
        node->RotationOrder.Set(FbxEuler::eOrderXZY);
        break;
      case MTransformationMatrix::kYXZ:
        node->RotationOrder.Set(FbxEuler::eOrderYXZ);
        break;
      case MTransformationMatrix::kZYX:
        node->RotationOrder.Set(FbxEuler::eOrderZYX);
        break;

      default:
        node->RotationOrder.Set(FbxEuler::eOrderXYZ);
        break;
    }
    node->UpdatePivotsAndLimitsFromProperties();

    auto l_loc = l_transform.getTranslation(MSpace::kTransform, &l_status);
    maya_chick(l_status);
    node->LclTranslation.Set({l_loc.x, l_loc.y, l_loc.z});
    node->LclRotation.Set(FbxVector4{l_rot.x, l_rot.y, l_rot.z});
    std::double_t l_scale[3]{};
    l_transform.getScale(l_scale);
    node->LclScaling.Set({l_scale[0], l_scale[1], l_scale[2]});
    node->ScalingMax.Set({});
  }

  void write_joint(const MDagPath& in_mesh) {
    auto* l_sk_attr = FbxSkeleton::Create(node->GetScene(), "skeleton");
    l_sk_attr->SetSkeletonType(FbxSkeleton::eLimbNode);
    MStatus l_status{};
    auto l_is_ = get_plug(in_mesh.node(), "segmentScaleCompensate").asBool(&l_status);
    maya_chick(l_status);

    node->RotationActive.Set(true);
    auto l_vector_x = get_plug(in_mesh.node(), "jointOrientX").asMAngle(&l_status);
    maya_chick(l_status);
    auto l_vector_y = get_plug(in_mesh.node(), "jointOrientY").asMAngle(&l_status);
    maya_chick(l_status);
    auto l_vector_z = get_plug(in_mesh.node(), "jointOrientZ").asMAngle(&l_status);
    maya_chick(l_status);
    node->PreRotation.Set(FbxVector4{l_vector_x.asDegrees(), l_vector_y.asDegrees(), l_vector_z.asDegrees()});
    //      node->SetPreRotation(FbxNode::eSourcePivot, FbxVector4{l_vector_x, l_vector_y, l_vector_z});
    node->UpdatePivotsAndLimitsFromProperties();
    node->SetNodeAttribute(l_sk_attr);

    write_loc_attr(in_mesh);
    node->InheritType.Set(l_is_ ? FbxTransform::eInheritRrs : FbxTransform::eInheritRSrs);
  }

  void write_skeletion(const tree_mesh_t& in_tree, const MObject& in_skin);

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

class doodle_to_ue_fbx::impl_data {
 public:
 private:
 public:
  std::shared_ptr<FbxManager> manager_{};
  FbxScene* scene_{};
  tree_mesh_t tree_dag_{};

  std::vector<MDagPath> joints_{};

  void build_joint_tree() {
    for (auto&& i : joints_) {
      auto l_begin = tree_dag_.begin();
      for (std::int32_t j = i.length() - 1; j >= 0; --j) {
        MDagPath l_sub_path{i};
        l_sub_path.pop(j);

        if (auto l_tree_it = ranges::find_if(
                std::begin(l_begin), std::end(l_begin),
                [&](const tree_mesh_t::value_type& in_value) { return in_value.dag_path == l_sub_path; }
            );
            l_tree_it != std::end(l_begin)) {
          l_begin = l_tree_it;
        } else {
          auto l_parent_node = l_begin->node;
          l_begin            = tree_dag_.append_child(l_begin, tree_dag_node{l_sub_path});
          l_begin->node      = FbxNode::Create(scene_, l_sub_path.partialPathName().asChar());
          l_parent_node->AddChild(l_begin->node);
          l_begin->write_file_ = [](tree_dag_node* self) {
            fbx_write_data l_data{self->node, nullptr};
            if (self->dag_path.hasFn(MFn::kJoint))
              l_data.write_joint(self->dag_path);
            else
              l_data.write_transform(self->dag_path);
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
                [&](const tree_mesh_t::value_type& in_value) { return in_value.dag_path == l_sub_path; }
            );
            l_tree_it != std::end(l_begin)) {
          l_begin = l_tree_it;
        } else {
          auto l_parent_node = l_begin->node;
          l_begin            = tree_dag_.append_child(l_begin, tree_dag_node{l_sub_path});
          l_begin->node      = FbxNode::Create(scene_, l_sub_path.partialPathName().asChar());
          l_parent_node->AddChild(l_begin->node);
          l_begin->skin_cluster_ = get_skin_custer(l_begin->dag_path);
          if (l_begin->dag_path.hasFn(MFn::kMesh) && !l_begin->skin_cluster_.isNull()) {
            l_begin->write_file_ = [this](tree_dag_node* self) {
              fbx_write_data l_data{self->node, nullptr};
              l_data.write_mesh(self->dag_path);
              l_data.write_skeletion(tree_dag_, self->skin_cluster_);
            };
          } else {
            l_begin->write_file_ = [](tree_dag_node* self) {
              fbx_write_data l_data{self->node, nullptr};
              l_data.write_transform(self->dag_path);
            };
          }
          if (!l_begin->skin_cluster_.isNull() && l_begin->skin_cluster_.hasFn(MFn::kSkinClusterFilter)) {
            MGlobal::displayInfo(conv::to_ms(fmt::format("写出皮肤簇 {}", l_begin->dag_path)));
            joints_ |= ranges::action::push_back(fbx_write_data::find_joint(l_begin->skin_cluster_));
          }
        }
      }
    }
  }

  void init() {
    tree_dag_ = {tree_dag_node{MDagPath{}, scene_->GetRootNode(), MObject::kNullObj, [](auto...) {}, nullptr}};
  }

  void build_tree(const MSelectionList& in_list) {
    build_mesh_tree(in_list);
    build_joint_tree();
  }
  void write() { iter_tree(tree_dag_.begin()); }

  void iter_tree(const tree_mesh_t ::iterator& in_iterator) {
    for (auto i = in_iterator.begin(); i != in_iterator.end(); ++i) {
      i->write();
      iter_tree(i);
    }
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

void fbx_write_data::write_skeletion(const tree_mesh_t& in_tree, const MObject& in_skin) {
  if (mesh == nullptr) {
    log_error(fmt::format(" {} is not mesh", node->GetName()));
    return;
  }
  auto* l_sk = FbxSkin::Create(node->GetScene(), "skin");
  mesh->AddDeformer(l_sk);

  auto l_joint_list = find_joint(in_skin);

  MFnSkinCluster l_skin_cluster{in_skin};

  std::map<MDagPath, tree_mesh_t::iterator, details::cmp_dag> l_dag_tree_map{};
  for (auto l_it = in_tree.begin(); l_it != in_tree.end(); ++l_it) {
    if (ranges::find_if(l_joint_list, boost::lambda2::_1 == l_it->dag_path) != std::end(l_joint_list)) {
      l_it->write();
      l_dag_tree_map.emplace(l_it->dag_path, l_it);
    }
  }
  std::map<MDagPath, FbxCluster*, details::cmp_dag> l_dag_fbx_map{};

  for (auto&& i : l_joint_list) {
    auto l_joint    = l_dag_tree_map[i];
    auto* l_cluster = FbxCluster::Create(node->GetScene(), "");
    l_cluster->SetLink(l_joint->node);
    l_cluster->SetLinkMode(FbxCluster::eTotalOne);
    l_cluster->SetTransformMatrix(node->EvaluateGlobalTransform());
    l_cluster->SetTransformLinkMatrix(l_joint->node->EvaluateGlobalTransform());
    l_dag_fbx_map.emplace(i, l_cluster);
    if (!l_sk->AddCluster(l_cluster)) {
      log_error(fmt::format("add cluster error: {}", node->GetName()));
    }
  }

  MStatus l_status{};
  MDagPath l_skin_mesh_path{};
  for (auto i = 0; i < l_skin_cluster.numOutputConnections(); ++i) {
    auto l_index = l_skin_cluster.indexForOutputConnection(i, &l_status);
    maya_chick(l_status);
    maya_chick(l_skin_cluster.getPathAtIndex(l_index, l_skin_mesh_path));

    MItGeometry l_it_geo{l_skin_mesh_path};
    log_info(fmt::format("写出皮肤簇 {} 顶点数 {}", l_skin_mesh_path, l_it_geo.count()));
    for (; !l_it_geo.isDone(); l_it_geo.next()) {
      auto l_com = l_it_geo.currentItem(&l_status);
      maya_chick(l_status);
      std::uint32_t l_influence_count{};
      MDoubleArray l_influence_weights{};
      maya_chick(l_skin_cluster.getWeights(l_skin_mesh_path, l_com, l_influence_weights, l_influence_count));
      // 写出权重
      for (auto j = 0; j < l_influence_count; ++j) {
        if (l_influence_weights[j] == 0) continue;
        auto l_cluster = l_dag_fbx_map[l_joint_list[j]];
        l_cluster->AddControlPointIndex(l_it_geo.index(), l_influence_weights[j]);
      }
    }
    break;
  }
  {  // build post
    auto* l_post = FbxPose::Create(node->GetScene(), fmt::format("{}_post", get_node_name(in_skin)).c_str());
    l_post->SetIsBindPose(true);
    std::vector<tree_mesh_t::iterator> post_add{};

    std::function<bool(tree_mesh_t::iterator)> l_iter{};
    l_iter = [&](tree_mesh_t::iterator in_parent) -> bool {
      bool l_r{};
      for (auto l_it = in_parent.begin(); l_it != in_parent.end(); ++l_it) {
        auto l_sub_has = l_iter(l_it);
        if (ranges::find_if(l_joint_list, boost::lambda2::_1 == l_it->dag_path) != std::end(l_joint_list) ||
            l_it->dag_path == l_skin_mesh_path || l_sub_has) {
          post_add.emplace_back(l_it);
          l_r |= true;
        }
      }
      return l_r;
    };

    l_iter(in_tree.begin());

    for (auto&& i : post_add) {
      l_post->Add(i->node, i->node->EvaluateGlobalTransform());
    }
    node->GetScene()->AddPose(l_post);
  }
  mesh->AddDeformer(l_sk);
}

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
  p_i->write();
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