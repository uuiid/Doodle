//
// Created by TD on 2021/6/17.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/core/observable_container.h>

namespace doodle {

class DOODLELIB_API doodle_lib : public details::no_copy {
  friend DOODLELIB_API DoodleLibPtr make_doodle_lib();

  static doodle_lib* p_install;
  doodle_lib();

  ThreadPoolPtr p_thread_pool;
  ProjectPtr p_curr_project;
  RpcMetadataClientPtr p_rpc_metadata_clien;
  RpcFileSystemClientPtr p_rpc_file_system_client;
  MetadataFactoryPtr p_metadata_factory;

  FSys::path create_time_database();

 public:
  virtual ~doodle_lib();

  static doodle_lib& Get();

  void set_thread_pool_size();
  ThreadPoolPtr get_thread_pool();

  using project_vector = std::vector<ProjectPtr>;
  observable_container<project_vector> p_project_vector;

  void init_gui();

  [[nodiscard]] RpcMetadataClientPtr get_rpc_metadata_client() const;
  [[nodiscard]] RpcFileSystemClientPtr get_rpc_file_system_client() const;
  [[nodiscard]] MetadataFactoryPtr get_metadata_factory() const;

  std::vector<long_term_ptr> long_task_list;
  std::recursive_mutex mutex;

};
}  // namespace doodle
