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
  using flag_type    = std::bitset<3>;
  using sig_type     = boost::signals2::signal<void(const Data_Type&)>;
  using solt_type    = typename sig_type::slot_type;
  using connect_type = boost::signals2::connection;

 private:
  sig_type sig_attr;

  flag_type flag;
  bool current_flag{false};

  void call_fun(const Data_Type& in_data) {
    if (chick_call()) {
      sig_attr(in_data);
      flag.reset();
    }
  }

  bool chick_call() {
    const static flag_type s_f1{0b001};
    const static flag_type s_f2{0b101};
    const static flag_type s_f3{0b011};
    auto l_chick = (flag ^ s_f1).all() ||
                   (flag ^ s_f2).all() ||
                   (flag ^ s_f3).all();
    return l_chick;
  }

 public:
  modify_guard()          = default;
  virtual ~modify_guard() = default;

  /**
   * @brief 链接信号
   * @param in_solt_type 槽类型
   * @return 链接类
   */
  connect_type connect(const solt_type& in_solt_type) {
    return sig_attr.connect(in_solt_type);
  }

  /**
   * @brief 调用信号
   * @param in_data
   */
  void operator()(const Data_Type& in_data) {
    call_fun(in_data);
  }
  /**
   * @brief 传入检查是否修改的 bool 类型便利函数
   * @param in 是否修改段结果
   * @return 本身
   */
  modify_guard& operator=(bool in) {
    current_flag = in;
    if (in)
      flag[0] = in;
    return *this;
  }
  /**
   * @brief 检查当前是否进行修改
   * @return 是否修改
   */
  explicit operator bool() {
    return chick_call();
  }

  /**
   * @brief 测试当前帧是否进行过修改
   * @return 是否修改
   */
  bool current_frame_modify() {
    return flag[0];
  }
  /**
   * @brief 测试当前传入的变量是否进行了修改
   * @return 是否修改
   */
  bool current_modify() {
    return current_flag;
  }

  /**
   * @brief 开始检查
   */
  void begin_flag() {
    flag <<= 1;
  };
  /**
   * @brief 结束检查
   * @warning 如果没有在渲染函数以外进行异步修改不需要调用
   */
  void async_begin_flag() {
    flag <<= 1;
  }
};
}  // namespace doodle::gui
