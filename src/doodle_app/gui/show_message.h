//
// Created by TD on 2022/10/28.
//

#pragma once

#include <doodle_app/doodle_app_fwd.h>
#include <doodle_app/gui/base/base_window.h>

namespace doodle {
namespace gui {

class show_message : public base_windows<dear::PopupModal, show_message> {
  class impl;
  std::unique_ptr<impl> p_i;
  std::string Message;

 public:
  explicit show_message();
  virtual ~show_message() override;
  void set_attr() const;
  std::int32_t flags() const;
  void render();

  void set_message(const std::string& in_msg);  /// 设置消息内容
  std::string get_message();
};

}  // namespace gui
}  // namespace doodle
