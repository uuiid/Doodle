//
// Created by TD on 2021/6/17.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/long_task/process_pool.h>
namespace doodle {

class DOODLELIB_API doodle_lib : public details::no_copy {
  static doodle_lib* p_install;

  thread_pool_ptr p_thread_pool;
  logger_ctr_ptr p_log;
  FSys::path create_time_database();

 public:
  doodle_lib();
  virtual ~doodle_lib();

  static doodle_lib& Get();


  thread_pool_ptr get_thread_pool();

  scheduler_t loop;
  bounded_pool_t loop_bounded_pool;
  void init_gui();

  std::vector<long_term_ptr> long_task_list;
  std::recursive_mutex mutex;

  registry_ptr reg;
};
DOODLELIB_API inline registry_ptr& g_reg() {
  return doodle_lib::Get().reg;
}
DOODLELIB_API inline scheduler_t& g_main_loop() {
  return doodle_lib::Get().loop;
}
DOODLELIB_API inline bounded_pool_t &g_bounded_pool(){
  return doodle_lib::Get().loop_bounded_pool;
}

}  // namespace doodle
