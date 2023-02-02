//
// Created by TD on 2022/5/30.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/platform/win/windows_alias.h>

#include <thread>

namespace doodle {
/**
 * @brief 基础的事件循环类,  只有事件循环可以使用
 */

namespace detail {
class app_facet_interface;
}  // namespace detail
/**
 * @brief app 的基类
 *
 */
class DOODLE_CORE_API app_base {
 public:
  using cmd_string_type = std::variant<win::string_type, std::vector<std::string>>;
  using app_facet_ptr   = std::shared_ptr<::doodle::detail::app_facet_interface>;
  using app_facet_map   = std::map<std::string, app_facet_ptr>;

 protected:
  static app_base* self;
  doodle_lib_ptr lib_ptr;

  std::wstring p_title;
  app_facet_map facet_list{};
  /**
   * 此处更改为默认运行构面的名称
   */
  std::string default_run_facet_name{};
  /// @brief 在初始化中获取的id为主id(也是渲染线程id)
  std::thread::id run_id{std::this_thread::get_id()};

  void init();

 protected:
  /**
   * @brief 这个会在第一个循环中加载
   *
   */

  virtual void post_constructor()    = 0;
  virtual bool chick_authorization() = 0;
  virtual void deconstruction()      = 0;
  std::atomic_bool stop_;

 public:
  app_base();
  virtual ~app_base();

  /**
   * @brief 直接使用默认配置运行
   * @return
   */
  virtual std::int32_t run();

  bool is_main_thread() const;

  virtual std::int32_t poll_one();
  std::atomic_bool& stop();
  void stop_app(bool in_stop = false);

  void load_project(const FSys::path& in_path) const;

  DOODLE_DIS_COPY(app_base);
  static app_base& Get();
};

}  // namespace doodle
