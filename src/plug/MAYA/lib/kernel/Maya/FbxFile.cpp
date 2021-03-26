#include <lib/kernel/Maya/FbxFile.h>
#include <lib/kernel/Exception.h>

#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MItDag.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>

// #include <fbxsdk.h>
#include <fstream>
namespace doodle::motion::kernel {

FbxFile::FbxFile(FSys::path path) {
  // FbxManager* manager = FbxManager::Create();
  // if (!manager) throw DoodleError("无法初始化fbx经理");
  // MString str{"Autodesk FBX SDK version ^1s\n"};
  // str.format(manager->GetVersion());
  // MGlobal::displayInfo(str);

  // auto iosetting = FbxIOSettings::Create(manager, IOSROOT);
  // auto scene     = FbxScene::Create(manager, "Doodle_aim");
  // if (!scene) throw DoodleError("无法创建场景");

  // auto export_ = FbxExporter::Create(manager, "");
  // export_->Initialize("");
}

void FbxFile::FbxExportMEL(FSys::path path) {
  MStatus status = MStatus::MStatusCode::kFailure;

  MSelectionList k_list{};
  auto k_has_bone = false;
  auto k_dag_it   = MItDag{};
  auto k_dag_path = MDagPath{};

  status = MGlobal::getActiveSelectionList(k_list);
  if (status != MStatus::MStatusCode::kSuccess) throw FbxFileError("获得选择失败");

  if (k_list.isEmpty()) throw FbxFileError("没有选择对象");
  if (k_list.length() != 1) MGlobal::displayError("选择对象过多, 使用第一个选择对象");

  status = k_list.getDagPath(0, k_dag_path);
  if (status != MStatus::MStatusCode::kSuccess) throw FbxFileError("第一个选择不是dag对象");

  for (k_dag_it.reset(k_dag_path); !k_dag_it.isDone(); k_dag_it.next()) {
    auto dag_it = MDagPath{};
    status      = k_dag_it.getPath(dag_it);
    if (status != MStatus::MStatusCode::kSuccess)
      throw FbxFileError("获得路径失败");
    auto dag_node_fn = MFnDagNode{};
    if (k_dag_path.apiType(&status) == MFn::Type::kJoint) {
      if (status != MStatus::MStatusCode::kSuccess)
        throw FbxFileError("获取类别失败");
      k_has_bone = true;
      break;
    }
  }

  if (!k_has_bone)
    throw FbxFileError("选择中没有骨骼");

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

  status = k_file_path.setUTF8(path.generic_string().c_str());
  if (status != MStatus::MStatusCode::kSuccess) throw FbxFileError("生成路径失败");

  status = k_mel.format(k_mel, k_file_path);
  if (status != MStatus::MStatusCode::kSuccess) throw FbxFileError("格式化命令失败");

  MGlobal::displayInfo(k_mel);

  status = MGlobal::executeCommand(k_mel);
  if (status != MStatus::MStatusCode::kSuccess) throw FbxFileError("执行命令失败");
}

void FbxFile::FbxImportMEL(FSys::path path) {
  MStatus status = MStatus::MStatusCode::kFailure;

  MString k_mel{};
  k_mel += R"(loadPlugin "fbxmaya";
FBXImportCacheFile -v false;
FBXImportCameras -v false;
FBXImportConstraints -v false;
FBXImportLights -v false;
FBXImportShapes -v false;
FBXImportMode -v add;

FBXImport -file "^1s";
)";
  MString k_file_path{};

  status = k_file_path.setUTF8(path.generic_string().c_str());
  if (status != MStatus::MStatusCode::kSuccess) throw FbxFileError("生成路径失败");

  status = k_mel.format(k_mel, k_file_path);
  if (status != MStatus::MStatusCode::kSuccess) throw FbxFileError("格式化命令失败");

  MGlobal::displayInfo(k_mel);

  status = MGlobal::executeCommand(k_mel);
  if (status != MStatus::MStatusCode::kSuccess) throw FbxFileError("执行命令失败");
}

FbxFile::~FbxFile() {
}

}  // namespace doodle::motion::kernel