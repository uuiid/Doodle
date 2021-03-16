#include <lib/kernel/Maya/FbxExport.h>
#include <lib/kernel/Exception.h>

#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
// #include <fbxsdk.h>
#include <fstream>
namespace doodle::motion::kernel {

FbxExport::FbxExport(FSys::path path) {
  // FbxManager* manager = FbxManager::Create();
  // if (!manager) throw std::runtime_error("无法初始化fbx经理");
  // MString str{"Autodesk FBX SDK version ^1s\n"};
  // str.format(manager->GetVersion());
  // MGlobal::displayInfo(str);

  // auto iosetting = FbxIOSettings::Create(manager, IOSROOT);
  // auto scene     = FbxScene::Create(manager, "Doodle_aim");
  // if (!scene) throw std::runtime_error("无法创建场景");

  // auto export_ = FbxExporter::Create(manager, "");
  // export_->Initialize("");
}

MStatus FbxExport::FbxExportMEL(FSys::path path) {
  MStatus status = MStatus::MStatusCode::kFailure;

  MSelectionList k_list{};

  status = MGlobal::getActiveSelectionList(k_list);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  if (k_list.isEmpty()) throw FbxFileError("没有选择对象");

  MString k_mel{};
  k_mel += R"(loadPlugin "fbxmaya";
FBXExportInputConnections -v false;
FBXExportConstraints -v false;
FBXExportCameras -v false;
FBXExportBakeComplexAnimation  -v true;
FBXExportSkeletonDefinitions  -v true;
FBXExportSkins -v false;
FBXExportShapes -v false;
FBXExportLights -v false;
FBXExportInstances -v false;
FBXExport -f "^1s" -s;
)";
  MString k_file_path{};

  status = k_file_path.setUTF8(path.generic_u8string().c_str());
  CHECK_MSTATUS_AND_RETURN_IT(status);

  status = k_mel.format(k_mel, k_file_path);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  MGlobal::displayInfo(k_mel);

  status = MGlobal::executeCommand(k_mel);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  return status;
}

FbxExport::~FbxExport() {
}

}  // namespace doodle::motion::kernel