#pragma once

#include <lib/MotionGlobal.h>

#include <Maya/MGlobal.h>
namespace doodle::motion::kernel {
class FbxExport {
 public:
  FbxExport(FSys::path path);
  static MStatus FbxExportMEL(FSys::path path);
  ~FbxExport();
};
}  // namespace doodle::motion::kernel