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


class DOODLE_CORE_API doodle_lib : public details::no_copy,
                                      boost::equality_comparable<doodle_lib> {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  doodle_lib();
  virtual ~doodle_lib();

  static doodle_lib& Get();
  static FSys::path create_time_database();

  [[nodiscard]] thread_pool_ptr get_thread_pool() const;

  [[nodiscard]] registry_ptr& reg_attr() const;
  [[nodiscard]] scheduler_t& main_loop_attr() const;
  [[nodiscard]] bounded_pool_t& bounded_pool_attr() const;
  [[nodiscard]] boost::asio::io_context& io_context_attr() const;
  [[nodiscard]] boost::asio::thread_pool& thread_attr() const;

  bool operator==(const doodle_lib& in_rhs) const;
};

}  // namespace doodle
