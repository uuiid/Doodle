//
// Created by TD on 2022/8/1.
//

#pragma once

#include <doodle_app/doodle_app_fwd.h>

#include <bitset>
namespace doodle::gui::detail {

/**
 * @brief 可以进行跨越多帧进行检查的类(检查是否在拖拽等, 并在结束时进行回调)
 * @tparam Cache_T 传入的需要缓存的数据
 */
template <typename Cache_T = void>
class cross_frame_check {
 public:
  Cache_T data{};
  std::bitset<3> flag;
  using sig_type     = boost::signals2::signal<void(const Cache_T&)>;
  using solt_type    = typename sig_type::slot_type;
  using connect_type = boost::signals2::connection;

 private:
  sig_type call_fun{};
  constexpr const static std::bitset<3> flag_init{0b001};

  /**
   * @brief 开始初始帧
   */
  void begin_lock() {
    flag <<= 1;
  }
  /**
   * @brief 检查是否多帧连续成功
   *
   * 在这里调用回调
   */
  void begin_unlock() {
    auto l_f = flag ^ flag_init;
    if (l_f.all()) {
      call_fun(data);
      flag.reset();
    }
  }
  /**
   * @brief 在成功时记录此帧
   */
  void modify_lock() {
    flag |= flag_init;
  }

 public:
  /**
   * @brief 作用域守卫物体 非自动检查;
   */
  class guard_lock {
   protected:
    cross_frame_check& check_p;
    bool flag{};

   public:
    /**
     * @brief 强制传入检查类
     * @param in_check
     */
    explicit guard_lock(cross_frame_check& in_check)
        : check_p(in_check) {}

    /**
     * @brief 结束时检查守卫
     */
    virtual ~guard_lock() = default;

    /**
     * @brief 传入是否成功
     * @param in_bool 成功或者失败的标志
     * @return 自身
     */
    guard_lock& operator=(bool in_bool) {
      if (in_bool)
        check_p.modify_lock();
      flag = in_bool;
      return *this;
    }

    /**
     * @brief 传入需要缓存的数据 在发出信号时调用
     * @param in_data 传入的数据
     * @return 自身
     */
    guard_lock& operator^(const Cache_T& in_data) const {
      check_p = in_data;
      return *this;
    }
    /**
     * @copybrief guard_lock& operator^(const Cache_T& in_data) const
     */
    guard_lock& operator^(const Cache_T& in_data) {
      check_p = in_data;
      return *this;
    }
    /**
     * @brief 可以进行判断成功
     * @return
     */
    explicit operator bool() const {
      return flag;
    }
    /**
     * @brief 进行检查
     */
    void chick() const {
      this->check_p.begin_unlock();
    }
  };

 private:
  /**
   * @brief 作用域守卫物体(自动进行检查)
   */
  class guard_lock_auto : public guard_lock {
   public:
    using guard_lock::guard_lock;
    using guard_lock::operator=;
    using guard_lock::operator^;
    using guard_lock::operator bool;
    virtual ~guard_lock_auto() override {
      this->chick();
    }
  };

  /**
   * @brief 方便的数据转换类
   * @param in 传入的数据
   * @return
   */
  cross_frame_check& operator=(const Cache_T& in) {
    data = in;
    return *this;
  }

 public:
  cross_frame_check()          = default;
  virtual ~cross_frame_check() = default;
  //  DOODLE_MOVE(cross_frame_check);

  /**
   * @brief 链接信号
   * @param in_slot_type 传入的可调用对象
   * @return 信号链接
   */
  connect_type connect(const solt_type& in_slot_type) {
    return call_fun.connect(in_slot_type);
  }

  /**
   * @brief 获取跨帧守卫(自动检查类型)
   * @return 返回跨帧守卫(自动检查守卫)
   */
  [[nodiscard("")]] guard_lock_auto operator()() {
    begin_lock();
    return guard_lock_auto{*this};
  }

  /**
   * @brief 获取跨帧守卫(非自动类型)
   * @return 返回跨帧守卫
   */
  [[nodiscard("")]] guard_lock get_guard() {
    begin_lock();
    return guard_lock{*this};
  }
};

/**
 * @brief void 模板的特化
 */
// template <>
// class cross_frame_check<void> {
//  public:
//   std::bitset<3> flag;
//   using sig_type     = boost::signals2::signal<void()>;
//   using solt_type    = typename sig_type::slot_type;
//   using connect_type = boost::signals2::connection;
//
//  private:
//   sig_type call_fun{};
//   constexpr const static std::bitset<3> flag_init{0b001};
//
//   inline void begin_lock() {
//     flag <<= 1;
//   }
//   /// \brief 在这里调用回调
//   inline void begin_unlock() {
//     auto l_f = flag ^ flag_init;
//     if (l_f.all()) {
//       call_fun();
//       flag.reset();
//     }
//   }
//   inline void modify_lock() {
//     flag |= flag_init;
//   }
//
//   class guard_lock {
//     cross_frame_check& check_p;
//     bool flag{};
//
//    public:
//     explicit guard_lock(cross_frame_check& in_check)
//         : check_p(in_check) {}
//
//     virtual ~guard_lock() {
//       check_p.begin_unlock();
//     }
//
//     inline guard_lock& operator=(bool in_bool) {
//       if (in_bool)
//         check_p.modify_lock();
//       flag = in_bool;
//       return *this;
//     }
//
//     inline explicit operator bool() const {
//       return flag;
//     }
//   };
//
//  public:
//   cross_frame_check()          = default;
//   virtual ~cross_frame_check() = default;
//   //  DOODLE_MOVE(cross_frame_check);
//
//   inline connect_type connect(const solt_type& in_slot_type) {
//     return call_fun.connect(in_slot_type);
//   }
//
//   [[nodiscard("")]] inline guard_lock operator()() {
//     begin_lock();
//     return guard_lock{*this};
//   }
// };

}  // namespace doodle::gui::detail
