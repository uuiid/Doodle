//
// Created by TD on 2021/5/7.
//

#include <DoodleLib/Metadata/MetadataFactory.h>
#include <DoodleLib/core/CoreSet.h>
#include <Exception/Exception.h>
#include <core/MetadataSet.h>
#include <rpc/RpcMetadataClient.h>
#include <Metadata/MetadataFactory.h>
#include <boost/numeric/conversion/cast.hpp>

namespace doodle {

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
  sig_projectAdd(Project_.get(), boost::numeric_cast<int>(p_project_list.size()) - 1);
}

[[maybe_unused]] const ProjectPtr &MetadataSet::Project_() const {
  return p_project;
}

void MetadataSet::setProject_(const ProjectPtr &Project_) {
  auto it = std::find_if(p_project_list.begin(), p_project_list.end(), [Project_](ProjectPtr &in_prj) {
    return Project_.get() == in_prj.get();
  });
  if (it == p_project_list.end()) {
    throw DoodleError("项目不在列表中");
  }
  p_project = Project_;
  sig_projectChange(p_project.get(), getIntex(it));
}
int MetadataSet::getIntex(const std::vector<ProjectPtr>::const_iterator &it) const {
  return boost::numeric_cast<int>(std::distance(p_project_list.begin(), it));
}

void MetadataSet::setProject_(const Project *Project_) {
  auto it = std::find_if(p_project_list.begin(), p_project_list.end(),
                         [Project_](ProjectPtr &prj) { return Project_ == prj.get(); });
  if (it != p_project_list.end()) {
    p_project.swap(*it);
    sig_projectChange(p_project.get(), getIntex(it));
  } else {
    throw DoodleError{"项目不在列表中"};
  }
}

void MetadataSet::deleteProject(const Project *Project_) {
  auto it = std::find_if(p_project_list.begin(), p_project_list.end(),
                         [Project_](ProjectPtr &prj) { return Project_ == prj.get(); });
  if (it != p_project_list.end()) {
    sig_Projectdelete(it->get(), getIntex(it));
    p_project_list.erase(it);
  } else {
    throw DoodleError{"无法找到项目"};
  }
}

int MetadataSet::getProjectIndex() const {
  auto it = std::find(p_project_list.begin(), p_project_list.end(), p_project);
  return getIntex(it);
}
void MetadataSet::clear() {
  p_project_list.clear();
}

void MetadataSet::init() {
//  p_project_list = CoreSet::getSet().get_metadata_factory()->getAllProject();
  p_project_list = std::make_shared<MetadataFactory>()->getAllProject();
  if (!p_project)
    return;

  auto it = std::find_if(p_project_list.begin(), p_project_list.end(),
                         [this](const ProjectPtr &in_prj) { return in_prj->getId() == this->p_project->getId(); });
  if (it != p_project_list.end())
    p_project = *it;
  else
    p_project.reset();
}

}  // namespace doodle
