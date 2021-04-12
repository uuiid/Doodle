#pragma once
#include <corelib/core_global.h>

namespace doodle {

class CORE_API Ue4Project {
  FSys::path p_ue_path;
  FSys::path p_ue_Project_path;
  ProjectPtr p_project;

 public:
  const static std::string Content;
  const static std::string ContentShot;

  Ue4Project(FSys::path project_path);
  void createShotFolder(const std::vector<ShotPtr>& inShotList);
};
}  // namespace doodle