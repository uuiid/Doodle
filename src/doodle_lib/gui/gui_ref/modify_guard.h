//
// Created by TD on 2022/8/5.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/signals2.hpp>
#include <boost/noncopyable.hpp>
namespace doodle::gui {
template <typename Data_Type>
class modify_guard : boost::noncopyable {
 public:
  Data_Type data{};
  using flag_type    = std::bitset<3>;
  using sig_type     = boost::signals2::signal<void(const Data_Type&)>;
  using solt_type    = typename sig_type::slot_type;
  using connect_type = boost::signals2::connection;

 private:
  sig_type sig_attr;

  flag_type flag;

  void call_fun(const Data_Type& in_data) {
    if (chick_call())
      sig_attr(in_data);
  }

  void move_flag() {
    flag <<= 1;
  };

  bool chick_call() {
    const static flag_type s_f1{0b001};
    const static flag_type s_f2{0b101};
    auto l_chick = (flag ^ s_f1) | (flag ^ s_f2);
    return l_chick.all();
  }

  class grard {
   public:
    modify_guard& self;
    explicit grard(modify_guard& in_modify_guard)
        : self(in_modify_guard) {
      self.move_flag();
    };

    virtual ~grard() {
      self.move_flag();
    }
  };

 public:
  modify_guard()          = default;
  virtual ~modify_guard() = default;

  connect_type connect(const solt_type& in_solt_type) {
    return sig_attr.connect(in_solt_type);
  }

  void operator()(const Data_Type& in_data) {
    call_fun(in_data);
  }

  modify_guard& operator=(bool in) {
    if (in)
      flag[0] = in;
    return *this;
  }
  explicit operator bool() {
    return flag[0];
  }
};
}  // namespace doodle::gui
