//
// Created by TD on 2021/9/15.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/imgui_warp.h>

namespace doodle {
/**
 * @brief 基本的窗口类
 *
 * @tparam Panel 窗口内容渲染过程
 */
template <class Panel>
class DOODLELIB_API base_window : public process_t<base_window<Panel>> {
 public:
  using self_type    = base_window<Panel>;
  using base_type    = process_t<base_window<Panel>>;
  using derived_type = Panel;

  /**
   * @brief Construct a new base window object
   *
   * @tparam Args
   * @param args 面板构造函数转发
   */
  template <class... Args>
  explicit base_window(Args&&... args)
      : panel_(std::forward<Args>(args)...) {}

  /**
   * @brief 面板渲染属性
   *
   */
  Panel panel_;
  /**
   * @brief 是否显示面板
   *
   */
  bool show{true};
  /**
   * @brief 初始化函数， 将自身添加到上下文中，并将显示值写入 coreset
   *
   */
  [[maybe_unused]] virtual void init() {
    g_reg()->template set<self_type&>(*this);
    core_set::getSet().widget_show[std::string{panel_.name}] = show;
    panel_.init();
  }
  /**
   * @brief 成功后调用
   *
   */
  [[maybe_unused]] virtual void succeeded() {
    g_reg()->template unset<self_type>();
    core_set::getSet().widget_show[std::string{panel_.name}] = show;
    panel_.succeeded();
  }
  /**
   * @brief 失败后调用
   *
   */
  [[maybe_unused]] virtual void failed() {
    g_reg()->template unset<self_type>();
    core_set::getSet().widget_show[std::string{panel_.name}] = show;
    panel_.failed();
  }
  /**
   * @brief 用户停止时调用
   *
   */
  [[maybe_unused]] virtual void aborted() {
    g_reg()->template unset<self_type>();
    core_set::getSet().widget_show[std::string{panel_.name}] = show;
    panel_.aborted();
  }
  /**
   * @brief 渲染函数
   *
   * @param in_dalta 传入的间隔时间
   * @param in_data 用户数据
   */
  [[maybe_unused]] virtual void update(typename base_type::delta_type in_dalta, void* in_data) {
    if (!show)
      this->succeed();

    dear::Begin{panel_.name.data(), &show} &&
        [&]() {
          panel_.update(in_dalta, in_data);
        };
  }
};
/**
 * @brief 创建窗口模板方法
 *
 * @tparam Panel 窗口内容类
 * @tparam Args
 * @param args 实例化内容类初始化转发
 * @return auto viod
 */
template <class Panel, class... Args>
auto make_windows(Args&&... args) {
  if (auto k_panel = g_reg()->try_ctx<Panel>(); !k_panel)
    g_main_loop().attach<base_window<Panel>>(std::forward<Args>(args)...);
}

/**
 * @brief 基本的窗口类
 *
 * @tparam Panel 窗口内容渲染过程
 */
template <class Panel>
class DOODLELIB_API modal_window {
 private:
  Panel* This() {
    return dynamic_cast<Panel*>(this);
  }

 protected:
  std::vector<std::function<void()>> begin_fun;

 public:
  modal_window()
      : show{true} {
    begin_fun.emplace_back([this]() {
      ImGui::OpenPopup(This()->title().data());
      ImGui::SetNextWindowSize({640, 360});
    });
  };
  virtual ~modal_window(){};
  bool show;
  /**
   * @brief 渲染函数
   *
   * @param in_dalta 传入的间隔时间
   * @param in_data 用户数据
   */
  [[maybe_unused]] virtual void update(const std::chrono::system_clock::duration& in_dalta, void* in_data) {
    for (auto&& i : begin_fun) {
      i();
    }
    begin_fun.clear();
    //    if (!show)
    //      This()->fail();

    dear::PopupModal{This()->title().data(), &show} &&
        [&]() {
          This()->update(in_dalta, in_data);
        };
  }

  void close() {
    imgui::CloseCurrentPopup();
  }
};
}  // namespace doodle
