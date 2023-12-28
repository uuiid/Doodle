//
// Created by TD on 2022/5/30.
//

#pragma once
#include <doodle_core/core/app_facet.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/platform/win/windows_alias.h>

#include <boost/signals2.hpp>

#include <argh.h>
#include <thread>

namespace doodle {
/**
 * @brief 基础的事件循环类,  只有事件循环可以使用
 */

/**
 * @brief app 的基类
 *
 */
class DOODLE_CORE_API app_base {
 public:
 protected:
  class cancellation_signals {
    std::list<boost::asio::cancellation_signal> sigs;
    std::mutex mtx;

   public:
    void emit(boost::asio::cancellation_type ct = boost::asio::cancellation_type::all);

    boost::asio::cancellation_slot slot();
  };
  static app_base* self;
  doodle_lib_ptr lib_ptr;

  std::wstring p_title;
  argh::parser arg_;
  std::vector<std::shared_ptr<void>> facets_;

  /// @brief 在初始化中获取的id为主id(也是渲染线程id)
  std::thread::id run_id{std::this_thread::get_id()};

  void init();

  /**
   * @brief 这个会在第一个循环中加载
   *
   */

  virtual void post_constructor() = 0;
  virtual void deconstruction()   = 0;
  std::atomic_bool stop_;

 public:
  explicit app_base(int argc, const char* const argv[]);
  app_base() : app_base(0, nullptr) {}
  virtual ~app_base();

  /**
   * @brief 直接使用默认配置运行
   * @return
   */
  virtual std::int32_t run();

  bool is_main_thread() const;

  virtual std::int32_t poll_one();

  boost::signals2::signal<void()> on_stop;
  cancellation_signals on_cancel;

  void stop_app(bool in_stop = false);

  DOODLE_DIS_COPY(app_base);
  static app_base& Get();
};

}  // namespace doodle
