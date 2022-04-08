//
// Created by TD on 2022/4/8.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
namespace doodle::gui {
/**
 * @brief 基本窗口
 */
class DOODLELIB_API base_window {
 public:
  base_window()                                   = default;
  virtual ~base_window()                          = default;
  /**
   * @brief 获取窗口标识
   * @return 窗口标识
   */
  [[nodiscard]] virtual std::string title() const = 0;
  /**
   * @brief (构造函数后)初始化
   */
  virtual void init()                             = 0;
  /**
   * @brief 成功结束后调用
   */
  virtual void succeeded()                        = 0;
  /**
   * @brief 失败结束后调用
   */
  virtual void failed()                           = 0;
  /**
   * @brief 主动结束后调用
   */
  virtual void aborted()                          = 0;
  /**
   * @brief 每帧渲染调用
   * @param in_duration 传入的时间间隔
   * @param in_data 传入的自定义数据
   */
  virtual void update(
      const chrono::system_clock::duration& in_duration,
      void* in_data) = 0;
};

class DOODLELIB_API window_panel : public base_window {
 protected:
  std::map<std::string, std::variant<std::string, bool, std::int64_t>> setting{};

 public:
  window_panel()           = default;
  ~window_panel() override = default;
  virtual void read_setting();
  virtual void save_setting() const;

  void init() override;
  void succeeded() override;
  void aborted() override;
};
template <class Panel>
class DOODLELIB_API modal_window : public base_window {
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
  virtual ~modal_window() override = default;
  /**
   * @brief 模态窗口是否显示
   */
  bool show;

  virtual void update(const std::chrono::system_clock::duration& in_dalta, void* in_data) override {
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

}  // namespace doodle::gui
