#pragma once
#include <corelib/core_global.h>

namespace doodle {

class CORE_API Ue4Project {
  FSys::path p_ue_path;
  FSys::path p_ue_Project_path;

 public:
  Ue4Project(FSys::path project_path);
  void createShotFolder(const int start, const int end);
};
}  // namespace doodle