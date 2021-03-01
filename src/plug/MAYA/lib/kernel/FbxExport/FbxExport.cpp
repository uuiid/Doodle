#include <lib/kernel/FbxExport/FbxExport.h>

#include <maya/MGlobal.h>

#include <fbxsdk.h>
#include <fstream>
namespace doodle::motion::kernel {

FbxExport::FbxExport() {
  FbxManager* manager = FbxManager::Create();
  if (!manager) throw std::runtime_error("无法初始化fbx经理");
  MString str{"Autodesk FBX SDK version ^1s\n"};
  str.format(manager->GetVersion());
  MGlobal::displayInfo(str);

  auto iosetting = FbxIOSettings::Create(manager, IOSROOT);
  auto scene     = FbxScene::Create(manager, "Doodle_aim");
  if (!scene) throw std::runtime_error("无法创建场景");

  auto export_ = FbxExporter::Create(manager, "");
  export_->Initialize("");
}

FbxExport::~FbxExport() {
}

std::ostream& operator<<(std::ostream& os, const FbxExport& fbx) {
  os << "ok";

  return os;
}
}  // namespace doodle::motion::kernel