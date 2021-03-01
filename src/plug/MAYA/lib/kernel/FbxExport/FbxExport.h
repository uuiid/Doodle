#pragma once

#include <lib/MotionGlobal.h>

namespace doodle::motion::kernel {
class FbxExport {
 public:
  FbxExport();

  ~FbxExport();

  friend std::ostream& operator<<(std::ostream& os, const FbxExport& fbx);
};
std::ostream& operator<<(std::ostream& os, const FbxExport& fbx);
}  // namespace doodle::motion::kernel