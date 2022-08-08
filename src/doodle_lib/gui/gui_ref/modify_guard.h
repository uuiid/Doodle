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
  bool current_flag{false};

  void call_fun(const Data_Type& in_data) {
    if (chick_call()) {
      sig_attr(in_data);
      flag.reset();
    }
  }

  void begin_flag() {
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
      self.begin_flag();
    };

    virtual ~grard() {
      self.begin_flag();
    }
  };

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
   * @brief 获取守卫
   * @return 自动调用守卫
   */
  grard operator*() {
    return grard{*this};
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
};
}  // namespace doodle::gui
