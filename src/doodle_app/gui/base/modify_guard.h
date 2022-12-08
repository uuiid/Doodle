//
// Created by TD on 2022/8/5.
//

#pragma once
#include <doodle_app/doodle_app_fwd.h>

#include <boost/noncopyable.hpp>
#include <boost/signals2.hpp>

namespace doodle::gui {

class modify_guard {
 public:
  using flag_type = std::bitset<3>;

 private:
  flag_type flag;

  [[nodiscard]] bool chick_call() const {
    constexpr const static flag_type s_f1{0b001};
    constexpr const static flag_type s_f2{0b101};
    constexpr const static flag_type s_f3{0b011};
    auto l_chick = (flag ^ s_f1).all() || (flag ^ s_f2).all() || (flag ^ s_f3).all();
    return l_chick;
  }

 public:
  modify_guard()          = default;
  virtual ~modify_guard() = default;

  /**
   * @brief 传入检查是否修改的 bool 类型便利函数
   * @param in 是否修改段结果
   * @return 本身
   */
  modify_guard& operator=(bool in) {
    if (in) flag[0] = in;
    return *this;
  }
  /**
   * @brief 检查当前是否进行修改
   * @return 是否修改
   */
  operator bool() const { return chick_call(); }

  void modifyed() { flag[0] = true; }

  /**
   * @brief 测试当前帧是否进行过修改
   * @return 是否修改
   */
  bool current_frame_modify() const { return flag[0]; }

  /**
   * @brief 开始检查
   */
  void begin_flag() { flag <<= 1; };
};
}  // namespace doodle::gui
