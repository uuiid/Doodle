//
// Created by TD on 2022/1/22.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API screenshot_widget : public process_t<screenshot_widget> {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  /**
   * @brief 这个会将 iamge_icon组件附加到传入的 handle 上
   * @param in_handle
   */
  screenshot_widget(const entt::handle& in_handle);
  ~screenshot_widget() override;
  constexpr static std::string_view name{"screenshot_widget"};
  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(delta_type, void* data);
};

}  // namespace doodle
