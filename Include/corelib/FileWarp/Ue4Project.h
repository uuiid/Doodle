
#pragma once

#include <corelib/core_global.h>
#include <nlohmann/json.hpp>

#include <cereal/types/common.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/base_class.hpp>
#include <cereal/types/memory.hpp>
namespace doodle {
class CORE_API Ue4ProjectFilePulgins {
 public:
  std::string Name;
  bool Enabled;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(Ue4ProjectFilePulgins, Name, Enabled)
};

class CORE_API Ue4ProjectFile {
 public:
  int32_t FileVersion;
  std::string EngineAssociation;
  std::string Category;
  std::string Description;
  std::vector<Ue4ProjectFilePulgins> Plugins;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(Ue4ProjectFile, FileVersion,
                                 EngineAssociation,
                                 Category,
                                 Description,
                                 Plugins);
};

class CORE_API Ue4Project {
  FSys::path p_ue_path;
  FSys::path p_ue_Project_path;
  ProjectPtr p_project;

  void addUe4ProjectPlugins(const std::vector<std::string> &str) const;
  void runPythonScript(const std::string &python_str) const;
  void runPythonScript(const FSys::path &python_file) const;

 public:
  const static std::string Content;
  const static std::string ContentShot;
  const static std::string UE4PATH;
  Ue4Project(FSys::path project_path);
  void createShotFolder(const std::vector<ShotPtr> &inShotList);
};
}  // namespace doodle
