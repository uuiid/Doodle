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
DOODLE_CORE_EXPORT registry_ptr& g_reg();
DOODLE_CORE_EXPORT scheduler_t& g_main_loop();
DOODLE_CORE_EXPORT bounded_pool_t& g_bounded_pool();
DOODLE_CORE_EXPORT boost::asio::io_context& g_io_context();
DOODLE_CORE_EXPORT boost::asio::thread_pool& g_thread();

class DOODLE_CORE_EXPORT doodle_lib : public details::no_copy,
                                      boost::equality_comparable<doodle_lib> {
 private:
  static doodle_lib* p_install;
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  doodle_lib();
  virtual ~doodle_lib();

  static doodle_lib& Get();
  static FSys::path create_time_database();

  thread_pool_ptr get_thread_pool() const;

  registry_ptr& reg_attr() const;
  scheduler_t& main_loop_attr() const;
  bounded_pool_t& bounded_pool_attr() const;
  boost::asio::io_context& io_context_attr() const;
  boost::asio::thread_pool& thread_attr() const;

  bool operator==(const doodle_lib& in_rhs) const;
};

}  // namespace doodle
