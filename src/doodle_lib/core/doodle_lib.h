//
// Created by TD on 2021/6/17.
//

#pragma once

#include <doodle_lib/core/observable_container.h>
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {

class DOODLELIB_API doodle_lib : public details::no_copy {
  static doodle_lib* p_install;

  thread_pool_ptr p_thread_pool;
  logger_ctr_ptr p_log;
  rpc_metadata_client_ptr p_rpc_metadata_clien;
  rpc_file_system_client_ptr p_rpc_file_system_client;
  metadata_serialize_ptr p_metadata_factory;

  FSys::path create_time_database();

 public:
  doodle_lib();
  virtual ~doodle_lib();

  static doodle_lib& Get();
  virtual void post_constructor();

  void set_thread_pool_size();
  thread_pool_ptr get_thread_pool();

  std::vector<entt::entity> p_project_vector;
  scheduler_t loop;
  void init_gui();

  [[nodiscard]] rpc_metadata_client_ptr get_rpc_metadata_client() const;
  [[nodiscard]] rpc_file_system_client_ptr get_rpc_file_system_client() const;
  [[nodiscard]] metadata_serialize_ptr get_metadata_factory() const;

  std::vector<long_term_ptr> long_task_list;
  std::recursive_mutex mutex;

  registry_ptr reg;
};
DOODLELIB_API inline registry_ptr& g_reg() {
  return doodle_lib::Get().reg;
}
DOODLELIB_API inline scheduler_t& g_main_loop() {
  return doodle_app::Get()->loop;
}
}  // namespace doodle
