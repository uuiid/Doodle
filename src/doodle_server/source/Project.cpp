#include <doodle_server/source/Project.h>

#include <boost/filesystem.hpp>
DOODLE_NAMESPACE_S
Project::Project() {
}

std::shared_ptr<fileSys::path> Project::findPath(const std::string& project_name) const {
  auto path = p_project.find(project_name);
  if (path != p_project.end())
    return path->second;
  else
    return p_project.begin()->second;
}

Project& Project::Get() noexcept {
  static Project instance;
  return instance;
}

void Project::from_json(const nlohmann::json& j) {
  for (auto k_p : j.items()) {
    p_project.insert({k_p.key(),
                      std::make_shared<fileSys::path>(
                          k_p.value().get<std::string>())});
  }
}

void Project::to_json(nlohmann::json& j) {
  for (auto k_p : p_project) {
    j.push_back({k_p.first, k_p.second->generic_string()});
  }
}

Project::~Project() {
}
DOODLE_NAMESPACE_E
