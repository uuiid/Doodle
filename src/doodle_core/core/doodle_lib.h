//
// Created by TD on 2021/6/17.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/thread_pool/process_pool.h>

namespace boost::asio {
class io_context;
class thread_pool;
}  // namespace boost::asio

namespace doodle {

class DOODLE_CORE_EXPORT doodle_lib : public details::no_copy {
  static doodle_lib* p_install;

 public:
  doodle_lib();
  virtual ~doodle_lib();

  static doodle_lib& Get();
  FSys::path create_time_database();

  thread_pool_ptr p_thread_pool;
  logger_ctr_ptr p_log;
  registry_ptr reg;

  scheduler_t loop;
  bounded_pool_t loop_bounded_pool;

  std::shared_ptr<boost::asio::io_context> io_context_;

  thread_pool_ptr get_thread_pool();
};
DOODLE_CORE_EXPORT registry_ptr& g_reg();
DOODLE_CORE_EXPORT scheduler_t& g_main_loop();
DOODLE_CORE_EXPORT bounded_pool_t& g_bounded_pool();
DOODLE_CORE_EXPORT boost::asio::io_context& g_io_context();
DOODLE_CORE_EXPORT boost::asio::thread_pool& g_thread();

}  // namespace doodle
