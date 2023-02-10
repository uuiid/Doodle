//
// Created by TD on 2021/6/17.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <memory>

namespace boost::asio {
class io_context;
class thread_pool;
}  // namespace boost::asio

namespace doodle {

class DOODLE_CORE_API doodle_lib : public details::no_copy, boost::equality_comparable<doodle_lib> {
 public:
  using logger_ctr_ptr = std::shared_ptr<details::logger_ctrl>;
  friend boost::asio::io_context& g_io_context();
  friend boost::asio::thread_pool& g_thread();
  friend registry_ptr& g_reg();
  friend registry_ptr& g_reg();
  friend details::logger_ctrl& g_logger_ctrl();

 private:
  class impl;
  std::unique_ptr<impl> ptr;
  friend class core_set;
  void init();

 public:
  doodle_lib();
  virtual ~doodle_lib();

  static doodle_lib& Get();

  [[nodiscard]] registry_ptr& reg_attr() const;

  bool operator==(const doodle_lib& in_rhs) const;
};

}  // namespace doodle
