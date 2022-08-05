//
// Created by TD on 2022/8/5.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/signals2.hpp>

namespace doodle::gui {
template <typename Data_Type>
class modify_guard {
 public:
  Data_Type data{};
  std::bitset<3> flag;
  using sig_type     = boost::signals2::signal<void(const Data_Type&)>;
  using solt_type    = typename sig_type::slot_type;
  using connect_type = boost::signals2::connection;

 private:
  sig_type sig_attr;

  bool modify{false};

  void call_fun() {
  }

  class grard {
   public:
    modify_guard& self;
    explicit grard(modify_guard& in_modify_guard)
        : self(in_modify_guard){

          };

    virtual ~grard() {
      self.call_fun();
    }
  };

 public:
  modify_guard& operator=(bool in) {
    modify |= in;
    return *this;
  }
};
}  // namespace doodle::gui
