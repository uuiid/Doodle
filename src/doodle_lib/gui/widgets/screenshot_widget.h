//
// Created by TD on 2022/1/22.
//

#pragma once

#include <doodle_app/gui/base/base_window.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::gui {
class DOODLELIB_API screenshot_widget {
  class impl;
  std::unique_ptr<impl> p_i;
  bool open;
  using call_type     = std::function<void(const entt::handle&)>;
  using call_ptr_type = std::shared_ptr<call_type>;
  void handle_attr(const entt::handle& in);
  void call_save(const call_ptr_type& in);

  void succeeded();

 public:
  /**
   * @brief 这个会将 iamge_icon组件附加到传入的 handle 上
   * @param in_handle
   */
  screenshot_widget();
  ~screenshot_widget();
  constexpr static std::string_view name{"screenshot_widget"};

  bool render();

  void set_attr();
  const std::string& title() const;

  template <typename CompletionToken>
  auto async_save_image(const entt::handle& in, CompletionToken&& token) {
    return boost::asio::async_initiate<CompletionToken, void(const entt::handle& in)>(
        [this, in](auto&& completion_handler) {
          auto l_call = std::make_shared<call_type>(std::forward<decltype(completion_handler)>(completion_handler));
          this->handle_attr(in);
          this->call_save(l_call);
        },
        token
    );
  }
};

}  // namespace doodle::gui
