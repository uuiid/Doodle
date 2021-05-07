//
// Created by TD on 2021/5/7.
//

#include <corelib/core/MetadataSet.h>
#include <Exception/Exception.h>
#include <boost/numeric/conversion/cast.hpp>

namespace doodle{

MetadataSet::MetadataSet() {
}

MetadataSet &MetadataSet::Get() {
  static MetadataSet install;
  return install;
}

bool MetadataSet::hasProject() {
  return !p_project_list.empty();
}
std::vector<ProjectPtr> MetadataSet::getAllProjects() const {
  return p_project_list;
}

void MetadataSet::installProject(const ProjectPtr &Project_) {
  p_project_list.emplace_back(Project_);
}

[[maybe_unused]] const ProjectPtr &MetadataSet::Project_() const {
  if (!p_project)
    throw nullptr_error{"没有项目"};
  return p_project;
}

void MetadataSet::setProject_(const ProjectPtr &Project_) {
  p_project = Project_;
  auto it   = std::find(p_project_list.begin(), p_project_list.end(), Project_);
  if (it == p_project_list.end()) {
    p_project_list.emplace_back(Project_);
  }
}

void MetadataSet::setProject_(const Project *Project_) {
  auto it = std::find_if(p_project_list.begin(), p_project_list.end(),
                         [Project_](ProjectPtr &prj) { return Project_ == prj.get(); });
  if (it != p_project_list.end()) {
    p_project = *it;
  } else {
    throw DoodleError{"无法找到项目"};
  }
}

void MetadataSet::deleteProject(const Project *Project_) {
  auto it = std::find_if(p_project_list.begin(), p_project_list.end(),
                         [Project_](ProjectPtr &prj) { return Project_ == prj.get(); });
  if (it != p_project_list.end()) {
    p_project_list.erase(it);
  } else {
    throw DoodleError{"无法找到项目"};
  }
}

int MetadataSet::getProjectIndex() const {
  auto it    = std::find(p_project_list.begin(), p_project_list.end(), p_project);
  auto index = std::distance(p_project_list.begin(), it);
  return boost::numeric_cast<int>(index);
}
}
