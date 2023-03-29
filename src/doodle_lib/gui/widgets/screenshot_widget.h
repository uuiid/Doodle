//
// Created by TD on 2022/1/22.
//

#pragma once

#include <doodle_app/gui/base/base_window.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::gui {
class DOODLELIB_API screenshot_widget {
 public:
  using call_type     = std::function<void(const entt::handle&)>;
  using call_ptr_type = std::shared_ptr<call_type>;

 private:
  class impl;
  std::unique_ptr<impl> p_i;
  bool open;
  void handle_attr(const entt::handle& in);

  void succeeded();

 public:
  /**
   * @brief 这个会将 iamge_icon组件附加到传入的 handle 上
   * @param in_handle
   */
  screenshot_widget(const entt::handle& in_handle);
  ~screenshot_widget();
  static constexpr std::int32_t flags{
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove};
  constexpr static std::string_view name{"screenshot_widget"};

  bool render();

  void set_attr();
  const std::string& title() const;
};

}  // namespace doodle::gui
