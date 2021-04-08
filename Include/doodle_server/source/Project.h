#pragma once
#include <doodle_server/DoodleServer_global.h>
#include <map>
#include <nlohmann/json.hpp>

DOODLE_NAMESPACE_S

class Seting;

class Project {
 public:
  Project(const Project&) = delete;
  Project& operator=(const Project&) = delete;

  std::shared_ptr<fileSys::path> findPath(const std::string& project_name) const;

  static Project& Get() noexcept;

  friend class Seting;

 private:
  void from_json(const nlohmann::json& j);
  void to_json(nlohmann::json& j);

  Project();
  virtual ~Project();
  std::map<std::string, std::shared_ptr<fileSys::path>> p_project;
};

DOODLE_NAMESPACE_E