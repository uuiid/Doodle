//
// Created by td_main on 2023/9/12.
//

#include <doodle_app/app/program_options.h>

#include <doodle_lib/doodle_lib_all.h>

#include <Alembic/Abc/ArchiveInfo.h>
#include <Alembic/Abc/Foundation.h>
#include <Alembic/Abc/IArchive.h>
#include <Alembic/Abc/TypedArraySample.h>
#include <Alembic/AbcCoreAbstract/TimeSampling.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcCoreOgawa/ReadWrite.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcGeom/FaceSetExclusivity.h>
#include <Alembic/AbcGeom/Foundation.h>
#include <Alembic/AbcGeom/GeometryScope.h>
#include <Alembic/AbcGeom/IGeomBase.h>
#include <Alembic/AbcGeom/IPolyMesh.h>
#include <Alembic/AbcGeom/IXform.h>
#include <Alembic/AbcGeom/OFaceSet.h>
#include <Alembic/AbcGeom/OGeomParam.h>
#include <Alembic/AbcGeom/OPolyMesh.h>
#include <Alembic/AbcGeom/OXform.h>
#include <Alembic/AbcGeom/XformOp.h>
#include <Alembic/Util/PlainOldDataType.h>
#include <fbxsdk.h>
namespace doodle {
void write_fbx_impl(FbxScene* in_scene, const FSys::path& in_fbx_path) {
  auto* l_manager = in_scene->GetFbxManager();
  std::shared_ptr<FbxExporter> l_exporter{
      FbxExporter::Create(in_scene->GetFbxManager(), ""), [](FbxExporter* in_exporter) { in_exporter->Destroy(); }};
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_MATERIAL, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_TEXTURE, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_EMBEDDED, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_SHAPE, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_GOBO, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_ANIMATION, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_ASCIIFBX, false);

  if (l_exporter->Initialize(
          in_fbx_path.string().c_str(), l_manager->GetIOPluginRegistry()->GetNativeWriterFormat(),
          in_scene->GetFbxManager()->GetIOSettings()
      )) {
    DOODLE_LOG_ERROR("fbx exporter Initialize error: {}", l_exporter->GetStatus().GetErrorString());
  }
  l_exporter->Export(in_scene);
}
// 写入fbx
void write_fbx(const FSys::path& in_fbx_path, const Alembic::AbcGeom::IPolyMesh& in_poly) {
  std::shared_ptr<FbxManager> l_manager{FbxManager::Create(), [](FbxManager* in_manager) { in_manager->Destroy(); }};
  FbxIOSettings* l_io_settings = FbxIOSettings::Create(l_manager.get(), IOSROOT);
  l_manager->SetIOSettings(l_io_settings);
  FbxScene* l_scene{FbxScene::Create(l_manager.get(), "scene")};

  FbxDocumentInfo* l_doc_info{FbxDocumentInfo::Create(l_manager.get(), "info")};
  l_doc_info->mTitle   = "doodle fbx";
  l_doc_info->mSubject = "doodle fbx";
  l_doc_info->mAuthor  = "doodle";
  l_doc_info->Original_ApplicationVendor.Set("doodle");
  l_doc_info->Original_ApplicationName.Set("doodle");
  l_doc_info->Original_ApplicationVersion.Set("1.0.0");

  l_doc_info->LastSaved_ApplicationVendor.Set("doodle");
  l_doc_info->LastSaved_ApplicationName.Set("doodle");
  l_doc_info->LastSaved_ApplicationVersion.Set("1.0.0");

  l_scene->SetSceneInfo(l_doc_info);

  l_scene->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::cm);
  auto anim_stack = FbxAnimStack::Create(l_scene, "anim_stack");
  auto anim_layer = FbxAnimLayer::Create(l_scene, "anim_layer");
  anim_stack->AddMember(anim_layer);

  // 先写出节点
  auto* l_node = FbxNode::Create(l_scene, "node");
  l_scene->GetRootNode()->AddChild(l_node);

  // 写出网格
  auto* l_mesh         = FbxMesh::Create(l_scene, "mesh");
  l_node->SetNodeAttribute(l_mesh);

  const auto& l_sample = in_poly.getSchema();
  // 写出顶点
  auto l_sample_data   = l_sample.getValue();
  l_mesh->InitControlPoints(l_sample_data.getPositions()->size());
  auto* l_pos_list = l_mesh->GetControlPoints();
  for (std::size_t j = 0; j < l_sample_data.getPositions()->size(); ++j) {
    const auto& l_pos = (*l_sample_data.getPositions())[j];
    l_pos_list[j]     = FbxVector4{l_pos.x, l_pos.y, l_pos.z};
  }
  write_fbx_impl(l_scene, in_fbx_path);
}

void run(const FSys::path& in_abc_path) {
  Alembic::Abc::IArchive l_archive{Alembic::AbcCoreOgawa::ReadArchive(), in_abc_path.string()};
  auto l_top = l_archive.getTop();
  if (l_top.getNumChildren() == 0) {
    DOODLE_LOG_ERROR("abc file is empty");
    return;
  }

  for (auto i = 0; i < l_top.getNumChildren(); ++i) {
    auto l_child = l_top.getChild(i);
    l_child.getHeader();
    if (Alembic::AbcGeom::IXform::matches(l_child.getHeader())) {
      auto l_xform = Alembic::AbcGeom::IXform{l_child};
      DOODLE_LOG_INFO("xform {}", l_xform.getName());
      for (int l = 0; l < l_xform.getNumChildren(); ++l) {
        if (Alembic::AbcGeom::IPolyMesh::matches(l_xform.getChild(l).getHeader())) {
          auto l_poly = Alembic::AbcGeom::IPolyMesh{l_xform.getChild(l)};
          DOODLE_LOG_INFO("poly {}", l_poly.getName());
          write_fbx(in_abc_path.parent_path() / (l_poly.getName() + ".fbx"), l_poly);
          return;
        }
      }
    }
  }
}

}  // namespace doodle

int main(int argc, char* argv[]) {
  using namespace doodle;
  argh::parser l_parser(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);
  if (auto l_ss = l_parser("--abc"); l_ss) {
    FSys::path l_path{FSys::from_quotation_marks(l_ss.str())};
    l_path = l_path.make_preferred();
    run(l_path);
  } else {
    DOODLE_LOG_ERROR("abc path is empty");
  }
  return 0;
}