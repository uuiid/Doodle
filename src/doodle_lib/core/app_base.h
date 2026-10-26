//
// Created by TD on 2022/5/30.
//

#pragma once
#include <doodle_lib/core/cancellation_signals.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/platform/win/windows_alias.h>

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
class DOODLELIB_API app_base : public boost::noncopyable {
  friend DOODLELIB_API boost::asio::io_context& g_io_context();
  friend DOODLELIB_API entt::registry::context& g_ctx();
  friend DOODLELIB_API details::logger_ctrl& g_logger_ctrl();
  friend DOODLELIB_API boost::asio::strand<boost::asio::io_context::executor_type>& g_strand();
  friend DOODLELIB_API boost::asio::strand<boost::asio::io_context::executor_type>& g_pool_strand();

 public:
 protected:
  void bind_thread_to_group(int group_id);
  static app_base* self;

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
  void set_exit_code(std::int32_t in_exit_code);

 private:
  void init_lib();

  boost::asio::io_context io_context_{};
  entt::registry::context ctx_p{std::allocator<entt::entity>{}};
  using strand_t = boost::asio::strand<boost::asio::io_context::executor_type>;
  strand_t strand_{boost::asio::make_strand(io_context_)};
  std::vector<strand_t> strands_{};
  std::atomic_size_t strand_index_{0};
  std::shared_ptr<details::logger_ctrl> p_log{std::make_shared<std::shared_ptr<details::logger_ctrl>::element_type>()};

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
  inline bool is_cancelled() const { return stop_.load(); }

  // get argh
  inline const argh::parser& arg() const { return arg_; }

  cancellation_signals on_cancel;
  cancellation_signals on_stop_listener;

  virtual void stop_app(std::int32_t in_exit_code = 0);
  void clear_global_state();

  bool operator==(const app_base& in_rhs) const;

  static app_base& Get();
  static app_base* GetPtr();
};

}  // namespace doodle
