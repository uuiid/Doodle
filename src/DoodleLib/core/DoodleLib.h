//
// Created by TD on 2021/6/17.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/core/observable_container.h>

namespace doodle {

class DOODLELIB_API DoodleLib : public details::no_copy {
  friend DOODLELIB_API DoodleLibPtr make_doodle_lib();

  static DoodleLib* p_install;
  DoodleLib();

  ThreadPoolPtr p_thread_pool;
  ProjectPtr p_curr_project;

 public:
  virtual ~DoodleLib();

  static DoodleLib& Get();

  ThreadPoolPtr get_thread_pool();

  using project_vector = std::vector<ProjectPtr>;
  observable_container<project_vector> p_project_vector;

  const ProjectPtr& current_project() const;
  void set_current_project(const ProjectPtr& in_currProject);
};
}  // namespace doodle
