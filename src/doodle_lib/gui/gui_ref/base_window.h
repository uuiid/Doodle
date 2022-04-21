//
// Created by TD on 2022/4/8.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/long_task/process_pool.h>
#include <doodle_lib/core/init_register.h>
#include <boost/signals2.hpp>
#include <utility>
namespace doodle::gui {

/**
 * @brief 基本窗口
 */
class DOODLELIB_API base_window {
 protected:
  std::vector<std::function<void()>> begin_fun{};
  std::vector<boost::signals2::scoped_connection> sig_scoped{};

  bool show_{false};
  ImVec2 size_{};
  friend void to_json(nlohmann::json& j, const base_window& p);
  friend void from_json(const nlohmann::json& j, base_window& p);

 public:
  boost::signals2::signal<void()> close{};

  using list             = std::set<base_window*>;

  base_window()          = default;
  virtual ~base_window() = default;
  DOODLE_DIS_COPY(base_window)

  [[nodiscard]] nlohmann::json& get_setting() const;
  virtual void read_setting();
  virtual void save_setting() const;
  /**
   * @brief 获取窗口标识
   * @return 窗口标识
   */
  [[nodiscard]] virtual const string& title() const = 0;
  /**
   * @brief (构造函数后)初始化
   */
  virtual void init()                               = 0;
  /**
   * @brief 成功结束后调用
   */
  virtual void succeeded()                          = 0;
  /**
   * @brief 失败结束后调用
   */
  virtual void failed();
  /**
   * @brief 主动结束后调用
   */
  virtual void aborted();
  /**
   * @brief 每帧渲染调用
   * @param in_duration 传入的时间间隔
   * @param in_data 传入的自定义数据
   */
  virtual void update(
      const chrono::system_clock::duration& in_duration,
      void* in_data) = 0;
  /**
   * @brief 判断是否显示
   * @return
   */
  [[nodiscard]] bool is_show() const;
  void show(bool in_show = true);
  [[nodiscard]] virtual const ImVec2& size() const;
  virtual void size(const ImVec2& in_size);
  /**
   * @brief 安装窗口名称寻找窗口
   * @param in_title
   * @return
   */
  static base_window* find_window_by_title(const std::string& in_title);
};

class DOODLELIB_API windows_proc : public process_t<windows_proc> {
  std::optional<bool> optional_show{};

 public:
  class DOODLELIB_API warp_proc {
   public:
    bool show{};
    std::function<void()> close;

    [[nodiscard]] inline bool is_show() const {
      return show;
    };
  };
  using warp_proc_ptr = std::shared_ptr<warp_proc>;
  base_window* windows_;
  entt::meta_any owner_;
  std::shared_ptr<warp_proc> warp_proc_;

  explicit windows_proc(warp_proc_ptr in_ptr,
                        base_window* in_windows,
                        entt::meta_any&& in_meta_any)
      : windows_(in_windows),
        owner_(std::move(in_meta_any)),
        warp_proc_(std::move(in_ptr)) {
  }
  ~windows_proc() override;

  DOODLE_MOVE(windows_proc)
  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(
      const chrono::system_clock::duration& in_duration,
      void* in_data);
};

class DOODLELIB_API window_panel : public base_window {
 protected:
  std::string title_name_{};

  virtual void render() = 0;

 public:
  window_panel()           = default;
  ~window_panel() override = default;

  [[nodiscard]] const string& title() const override;
  void init() override;
  void succeeded() override;
  void aborted() override;
  void update(const std::chrono::system_clock::duration& in_dalta, void* in_data) override;
};

class DOODLELIB_API modal_window : public base_window {
 protected:
  virtual void render() = 0;

 public:
  modal_window();
  ~modal_window() override = default;

  void update(const std::chrono::system_clock::duration& in_dalta, void* in_data) override;
};

namespace base_windows_ns {
constexpr auto init_base_windows = []() {
  entt::meta<base_window>().type();
};

class init_base_windows_
    : public init_register::registrar_lambda<init_base_windows, 1> {};

constexpr auto init_windows_panel = []() {
  entt::meta<window_panel>().type().base<base_window>();
};

class init_windows_panel_
    : public init_register::registrar_lambda<init_windows_panel, 2> {};

}  // namespace base_windows_ns
}  // namespace doodle::gui
