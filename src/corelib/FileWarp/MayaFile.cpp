
#include <corelib/FileWarp/MayaFile.h>
DOODLE_NAMESPACE_S
MayaFile::MayaFile(fileSys::path path) : p_path(std::move(path)) {
}

std::string MayaFile::findMaya() {
  std::string mayaPY_path = "";
  if (boost::filesystem::exists(R"(C:\Program Files\Autodesk\Maya2020\bin)")) {
    mayaPY_path = R"(C:\Program Files\Autodesk\Maya2020\bin\)";
  } else if (boost::filesystem::exists(R"(C:\Program Files\Autodesk\Maya2019\bin)")) {
    mayaPY_path = R"(C:\Program Files\Autodesk\Maya2019\bin\)";
  } else if (boost::filesystem::exists(R"(C:\Program Files\Autodesk\Maya2018\bin)")) {
    mayaPY_path = R"(C:\Program Files\Autodesk\Maya2018\bin\)";
  } else {
    throw std::runtime_error("无法找到maya.exe");
  }
  return mayaPY_path;
}

bool MayaFile::exportFbxFile(fileSys::path path) {
  auto export_path = p_path.parent_path();
  if (!path.empty()) export_path = path;

  auto maya_exe = findMaya();
  return true;
}

bool MayaFile::checkFile() {
  return true;
}

DOODLE_NAMESPACE_E