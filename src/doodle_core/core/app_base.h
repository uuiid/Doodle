//
// Created by TD on 2022/5/30.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/platform/win/windows_alias.h>

namespace doodle {
/**
 * @brief 基础的事件循环类,  只有事件循环可以使用
 */

namespace app_base_ns {
class config_tick {
 public:
  std::function<void()> tick_begin;
  std::function<void()> tick_end;
};
}  // namespace app_base_ns

class DOODLE_CORE_EXPORT app_base {
 public:
  using cmd_string_type = std::variant<win::string_type, std::vector<std::string>>;

 protected:
  static app_base* self;

  doodle_lib_ptr p_lib;
  std::wstring p_title;
  win::wnd_instance instance;

 private:
  class impl;
  std::unique_ptr<impl> p_i;

  void init();

 protected:
  /**
   * @brief 这个会在第一个循环中加载
   *
   */
  virtual void load_back_end() = 0;
  virtual void loop_one();

  virtual void tick_begin();
  virtual void tick_end();

  virtual void post_constructor() = 0;

 public:
  class in_app_args {
   public:
    win::wnd_instance in_instance;
    cmd_string_type in_cmd_line;
  };

  explicit app_base();
  explicit app_base(const in_app_args& in_arg);
  virtual ~app_base();

  /**
   * @brief 直接使用默认配置运行
   * @return
   */
  virtual std::int32_t run();

  virtual void begin_loop();

  virtual std::int32_t poll_one();

  virtual void clear_loop();

  std::atomic_bool& stop();
  bool is_stop() const;
  void stop_app(bool in_stop = false);
  std::atomic_bool stop_;
  virtual bool valid() const;

  void load_project(const FSys::path& in_path) const;

  DOODLE_DIS_COPY(app_base);
  static app_base& Get();

  void _add_tick_(const std::function<void(bool&)>& in_tick);
};





}  // namespace doodle
