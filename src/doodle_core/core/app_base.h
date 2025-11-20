//
// Created by TD on 2022/5/30.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/platform/win/windows_alias.h>

#include <boost/asio/signal_set.hpp>
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

  argh::parser arg_;
  std::vector<std::shared_ptr<void>> facets_;

  /// @brief 在初始化中获取的id为主id(也是渲染线程id)
  std::thread::id run_id{std::this_thread::get_id()};
  std::atomic_bool stop_;

  std::int32_t exit_code{0};
  using signal_t = boost::asio::signal_set;

  std::shared_ptr<signal_t> sig_ptr;
  bool use_multithread_{false};
  /// 初始化函数, 返回true继续运行, 返回false退出
  virtual bool init();

 public:
  explicit app_base(int argc, const char* const argv[]);
  explicit app_base(std::int32_t argc, const wchar_t* const argv[]);
  app_base() : app_base(0, static_cast<const char* const[]>(nullptr)) {}
  virtual ~app_base();

  /**
   * @brief 直接使用默认配置运行
   * @return
   */
  virtual std::int32_t run();

  void add_signal();

  bool is_main_thread() const;
  void use_multithread(bool in_use = true);

  virtual std::int32_t poll_one();
  // get argh
  inline const argh::parser& arg() const { return arg_; }

  boost::signals2::signal<void()> on_stop;
  cancellation_signals on_cancel;

  virtual void stop_app(std::int32_t in_exit_code = 0);
  bool is_stopped() const { return stop_; }
  void set_stopped(bool in_stop) { stop_ = in_stop; }

  DOODLE_DIS_COPY(app_base);
  static app_base& Get();
  static app_base* GetPtr();

  static void write_current_error_tmp_dir();
};

/**
 * @brief 基本的命令行类
 */
template <typename... Facet_>
class app_command : public app_base {
 public:
  app_command() : app_base() { run_facet(); };

  app_command(int argc, const char* const argv[]) : app_base(argc, argv) { run_facet(); }
  explicit app_command(std::int32_t argc, const wchar_t* const argv[]) : app_base(argc, argv) { run_facet(); }
  virtual ~app_command() override = default;

  void run_facet() {
    try {
      std::array<bool, sizeof...(Facet_)> l_r{
          Facet_{}(arg_, facets_)...,
      };
      stop_ = std::any_of(l_r.begin(), l_r.end(), [](bool i) { return i; });
    } catch (...) {
      default_logger_raw()->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
      default_logger_raw()->flush();
      stop_ = true;
    }
  }

 protected:
};

template <typename... Facet_>
using app_plug = app_command<Facet_...>;

}  // namespace doodle
