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
  RpcMetadataClientPtr p_rpc_metadata_clien;
  RpcFileSystemClientPtr p_rpc_file_system_client;
  MetadataFactoryPtr p_metadata_factory;

  FSys::path create_time_database();

 public:
  virtual ~DoodleLib();

  static DoodleLib& Get();

  ThreadPoolPtr get_thread_pool();

  using project_vector = std::vector<ProjectPtr>;
  observable_container<project_vector> p_project_vector;

  void init_gui();

  [[nodiscard]] RpcMetadataClientPtr getRpcMetadataClient() const;
  [[nodiscard]] RpcFileSystemClientPtr getRpcFileSystemClient() const;

  std::vector<long_term_ptr> long_task_list;
  std::recursive_mutex mutex;
};
}  // namespace doodle
