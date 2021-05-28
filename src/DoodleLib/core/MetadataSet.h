//
// Created by TD on 2021/5/7.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <boost/signals2.hpp>
namespace doodle {
class DOODLELIB_API MetadataSet {
  MetadataSet();
  std::vector<ProjectPtr> p_project_list;
  std::shared_ptr<Project> p_project;

 public:
  static MetadataSet& Get();

  void Init();
  [[nodiscard]] bool hasProject();
  [[nodiscard]] std::vector<ProjectPtr> getAllProjects() const;
  void installProject(const ProjectPtr &Project_);
  [[nodiscard]] const ProjectPtr &Project_() const;
  void setProject_(const ProjectPtr &Project_);
  void setProject_(const Project *Project_);
  void deleteProject(const Project *Project_);
  [[nodiscard]] int getProjectIndex() const;

  void clear();
  DOODLE_DISABLE_COPY(MetadataSet)

  boost::signals2::signal<void (const Project *,int)> sig_Projectdelete;
  boost::signals2::signal<void (const Project *,int)> sig_projectChange;
  boost::signals2::signal<void (const Project *,int)> sig_projectAdd;
 private:
  //这里是序列化的代码
  friend class cereal::access;
  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version);
  [[nodiscard]] int getIntex(const std::vector<ProjectPtr>::const_iterator &it) const;
};
template <class Archive>
void MetadataSet::serialize(Archive &ar, const std::uint32_t version){
  ar(
      cereal::make_nvp("project",p_project)
      );
}
}  // namespace doodle
CEREAL_CLASS_VERSION(doodle::MetadataSet, 4);
