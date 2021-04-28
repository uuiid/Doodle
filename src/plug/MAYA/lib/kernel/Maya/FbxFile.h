#pragma once

#include <lib/MotionGlobal.h>

#include <Maya/MGlobal.h>
namespace doodle::motion::kernel {
class FbxFile {
 public:
  FbxFile(FSys::path path);
  static void FbxExportMEL(FSys::path path);
  static void FbxImportMEL(FSys::path path);
  ~FbxFile();
};
}  // namespace doodle::motion::kernel