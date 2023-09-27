//
// Created by td_main on 2023/9/26.
//

#include "doodle_to_ue_fbx.h"

#include <maya_plug/data/maya_tool.h>

#include <fbxsdk.h>
#include <maya/MArgDatabase.h>
#include <maya/MFloatArray.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItSelectionList.h>
#include <maya/MObjectArray.h>
#include <maya/MPointArray.h>
#include <maya/MSelectionList.h>

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
  FbxNode* node{};
  FbxMesh* mesh{};

  void write_mesh(const MObject& in_mesh) {
    if (!in_mesh.hasFn(MFn::kMesh)) {
      log_info("not mesh");
      return;
    }
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
    {
      // get uv set names
      MStringArray l_uv_set_names{};
      maya_chick(l_mesh.getUVSetNames(l_uv_set_names));

      for (auto&& i_name : l_uv_set_names) {
        auto l_uv_layer = mesh->GetLayer(mesh->CreateLayer());

        auto l_layer    = FbxLayerElementUV::Create(mesh, i_name.asChar());
        l_layer->SetMappingMode(FbxLayerElement::eByPolygonVertex);
        l_layer->SetReferenceMode(FbxLayerElement::eIndexToDirect);
        // for maya uv
        MFloatArray l_u{};
        MFloatArray l_v{};
        l_mesh.getUVs(l_u, l_v, &i_name);
        for (auto i = 0; i < l_u.length(); ++i) {
          l_layer->GetDirectArray().Add(FbxVector2{l_u[i], l_v[i]});
        }

        //        l_layer->GetIndexArray().SetCount(l_mesh.numVertices());
        auto l_face_count = l_mesh.numPolygons();
        for (auto i = 0; i < l_face_count; ++i) {
          MIntArray l_vert_list{};
          maya_chick(l_mesh.getPolygonVertices(i, l_vert_list));
          for (auto j = 0; j < l_vert_list.length(); ++j) {
            std::int32_t l_uv_id{};
            maya_chick(l_mesh.getPolygonUVid(i, j, l_uv_id, &i_name));
            l_layer->GetIndexArray().Add(l_uv_id);
          }
        }

        l_uv_layer->SetUVs(l_layer, FbxLayerElement::eTextureDiffuse);
      }
    }
  }
};

class doodle_to_ue_fbx::impl_data {
 public:
  std::shared_ptr<FbxManager> manager_{};
  FbxScene* scene_{};
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
  // for selectionlist
  MDagPath l_path{};
  for (MItSelectionList l_it{l_list}; !l_it.isDone(); l_it.next()) {
    maya_chick(l_it.getDagPath(l_path));
    maya_chick(l_path.extendToShape());

    auto l_node = FbxNode::Create(p_i->scene_, l_path.partialPathName().asChar());
    p_i->scene_->GetRootNode()->AddChild(l_node);
    fbx_write_data l_data{l_node, nullptr};
    l_data.write_mesh(l_path.node());
  }
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
  l_manager->GetIOSettings()->SetBoolProp(EXP_ASCIIFBX, false);

  if (!l_exporter->Initialize(
          //          in_fbx_path.string().c_str(),
          "D:/test.fbx", l_manager->GetIOPluginRegistry()->GetNativeWriterFormat(),
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