//
// Created by TD on 2021/5/7.
//

#pragma once
#include <corelib/core_global.h>
namespace doodle {
class CORE_API MetadataSet {
  MetadataSet();
  std::vector<ProjectPtr> p_project_list;
  std::shared_ptr<Project> p_project;

 public:
  static MetadataSet& Get();

  [[nodiscard]] bool hasProject();
  [[nodiscard]] std::vector<ProjectPtr> getAllProjects() const;
  void installProject(const ProjectPtr &Project_);
  [[nodiscard]] const ProjectPtr &Project_() const;
  void setProject_(const ProjectPtr &Project_);
  void setProject_(const Project *Project_);
  void deleteProject(const Project *Project_);
  [[nodiscard]] int getProjectIndex() const;

  DOODLE_DISABLE_COPY(MetadataSet)
};

}  // namespace doodle
