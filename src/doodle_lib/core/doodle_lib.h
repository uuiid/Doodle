//
// Created by TD on 2021/6/17.
//

#pragma once

#include <doodle_lib/core/observable_container.h>
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {

class DOODLELIB_API doodle_lib : public details::no_copy {
  friend DOODLELIB_API doodle_lib_ptr make_doodle_lib();

  static doodle_lib* p_install;
  doodle_lib();

  thread_pool_ptr p_thread_pool;
  project_ptr p_curr_project;
  rpc_metadata_client_ptr p_rpc_metadata_clien;
  rpc_file_system_client_ptr p_rpc_file_system_client;
  metadata_factory_ptr p_metadata_factory;

  FSys::path create_time_database();

 public:
  virtual ~doodle_lib();

  static doodle_lib& Get();

  void set_thread_pool_size();
  thread_pool_ptr get_thread_pool();

  using project_vector = std::vector<project_ptr>;
  observable_container<project_vector> p_project_vector;

  void init_gui();

  [[nodiscard]] rpc_metadata_client_ptr get_rpc_metadata_client() const;
  [[nodiscard]] rpc_file_system_client_ptr get_rpc_file_system_client() const;
  [[nodiscard]] metadata_factory_ptr get_metadata_factory() const;

  std::vector<long_term_ptr> long_task_list;
  std::recursive_mutex mutex;

};
}  // namespace doodle
