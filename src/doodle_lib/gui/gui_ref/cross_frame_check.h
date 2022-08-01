//
// Created by TD on 2022/8/1.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <bitset>
namespace doodle::gui::detail {

template <typename Cache_T>
class cross_frame_check {
 public:
  Cache_T data{};
  std::bitset<3> flag;

 private:
  boost::signals2::signal<void(const Cache_T&)> call_fun{};
  constexpr const static std::bitset<3> flag_init{0b001};

  void begin_lock() {
    flag <<= 1;
  }
  /// \brief 在这里调用回调
  void begin_unlock() {
    auto l_f = flag ^ flag_init;
    if (l_f.all()) {
      call_fun(data);
      flag.reset();
    }
  }
  void modify_lock() {
    flag |= flag_init;
  }

  class guard_lock {
    cross_frame_check& check_p;
    bool flag{};

   public:
    explicit guard_lock(cross_frame_check& in_check)
        : check_p(in_check) {}

    virtual ~guard_lock() {
      check_p.begin_unlock();
    }

    guard_lock& operator=(bool in_bool) {
      if (in_bool)
        check_p.modify_lock();
      flag = in_bool;
    }
    explicit operator bool() const {
      return flag;
    }
  };

 public:
  DOODLE_MOVE(cross_frame_check);

  cross_frame_check()          = default;
  virtual ~cross_frame_check() = default;
  guard_lock operator()() {
    begin_lock();
    return guard_lock{*this};
  }
};

}  // namespace doodle::gui::detail
