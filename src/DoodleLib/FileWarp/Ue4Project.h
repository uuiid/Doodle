
#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Logger/Logger.h>
#include <DoodleLib/threadPool/long_term.h>

#include <cereal/types/base_class.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <nlohmann/json.hpp>

namespace doodle {

class DOODLELIB_API Ue4Project {
  class DOODLELIB_API Ue4ProjectFilePulgins {
   public:
    std::string Name;
    bool Enabled;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Ue4ProjectFilePulgins, Name, Enabled)
  };
  class DOODLELIB_API Ue4ProjectFile {
   public:
    int32_t FileVersion{};
    std::string EngineAssociation;
    std::string Category;
    std::string Description;
    std::vector<Ue4ProjectFilePulgins> Plugins;

    // NLOHMANN_DEFINE_TYPE_INTRUSIVE(Ue4ProjectFile, FileVersion,
    //                                EngineAssociation,
    //                                Category,
    //                                Description,
    //                                Plugins);
    friend void to_json(nlohmann::json& nlohmann_json_j, const Ue4ProjectFile& nlohmann_json_t) {
      nlohmann_json_j["FileVersion"]       = nlohmann_json_t.FileVersion;
      nlohmann_json_j["EngineAssociation"] = nlohmann_json_t.EngineAssociation;
      nlohmann_json_j["Category"]          = nlohmann_json_t.Category;
      nlohmann_json_j["Description"]       = nlohmann_json_t.Description;
      nlohmann_json_j["Plugins"]           = nlohmann_json_t.Plugins;
    };
    friend void from_json(const nlohmann::json& nlohmann_json_j, Ue4ProjectFile& nlohmann_json_t) {
      nlohmann_json_j.at("FileVersion").get_to(nlohmann_json_t.FileVersion);
      nlohmann_json_j.at("EngineAssociation").get_to(nlohmann_json_t.EngineAssociation);
      nlohmann_json_j.at("Category").get_to(nlohmann_json_t.Category);
      nlohmann_json_j.at("Description").get_to(nlohmann_json_t.Description);
      try {
        nlohmann_json_j.at("Plugins").get_to(nlohmann_json_t.Plugins);
      } catch (const nlohmann::json::exception& error) {
        DOODLE_LOG_INFO(error.what());
      }
    };
  };

  FSys::path p_ue_path;
  FSys::path p_ue_Project_path;
  ProjectPtr p_project;
  long_term_ptr p_term;

  void addUe4ProjectPlugins(const std::vector<std::string>& in_strs) const;
  void runPythonScript(const std::string& python_str) const;
  void runPythonScript(const FSys::path& python_file) const;

 public:
  const static std::string Content;
  const static std::string ContentShot;
  const static std::string UE4PATH;
  const static std::string Character;
  const static std::string Prop;
  
  Ue4Project(FSys::path project_path);
  void createShotFolder(const std::vector<ShotPtr>& inShotList);
  long_term_ptr create_shot_folder_asyn(const std::vector<ShotPtr>& inShotList);

  static bool can_import_ue4(const FSys::path& in_path);
  static bool is_ue4_file(const FSys::path& in_path);
};
}  // namespace doodle
