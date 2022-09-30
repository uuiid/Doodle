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

namespace detail {
class app_facet_interface;
}  // namespace detail
class DOODLE_CORE_API app_base {
 public:
  using cmd_string_type = std::variant<win::string_type, std::vector<std::string>>;
  using app_facet_ptr   = std::shared_ptr<::doodle::detail::app_facet_interface>;

 protected:
  static app_base* self;

  doodle_lib_ptr p_lib;
  std::wstring p_title;
  win::wnd_instance instance;
  std::map<std::string, app_facet_ptr> facet_list{};

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
  std::atomic_bool stop_;

 public:
  class DOODLE_CORE_API in_app_args {
   public:
    win::wnd_instance in_instance{};
    cmd_string_type in_cmd_line{};
  };

  explicit app_base();
  explicit app_base(const in_app_args& in_arg);
  virtual ~app_base();

  void add_facet(const app_facet_ptr& in_facet);

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
  virtual bool valid() const;

  void load_project(const FSys::path& in_path) const;

  DOODLE_DIS_COPY(app_base);
  static app_base& Get();
};

}  // namespace doodle
