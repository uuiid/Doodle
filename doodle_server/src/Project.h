#pragma once
#include <DoodleServer_global.h>
#include <map>

DOODLE_NAMESPACE_S

class project {
 public:
  project(const project&) = delete;
  project& operator=(const project&) = delete;

  std::shared_ptr<boost::filesystem::path> findPath(const std::string& project_name) const;

  static project& Get() noexcept;

 private:
  project();
  virtual ~project();
  std::map<std::string, std::shared_ptr<boost::filesystem::path>> project;
};

DOODLE_NAMESPACE_E