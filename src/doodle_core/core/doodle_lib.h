//
// Created by TD on 2021/6/17.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
namespace boost::asio {
class io_context;
class thread_pool;
}  // namespace boost::asio

namespace doodle {

class DOODLE_CORE_API doodle_lib : public details::no_copy, boost::equality_comparable<doodle_lib> {
 private:
  class impl;
  std::unique_ptr<impl> ptr;
  friend class core_set;

  void clear();
  void init();

  struct guard {
    ~guard() { doodle_lib::Get().clear(); }
  };

 public:
  doodle_lib();
  virtual ~doodle_lib();

  static doodle_lib& Get();

  [[nodiscard]] registry_ptr& reg_attr() const;
  [[nodiscard]] boost::asio::io_context& io_context_attr() const;
  [[nodiscard]] boost::asio::thread_pool& thread_attr() const;

  bool operator==(const doodle_lib& in_rhs) const;
};

}  // namespace doodle
