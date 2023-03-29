//
// Created by TD on 2022/10/28.
//

#pragma once

#include <doodle_app/doodle_app_fwd.h>
#include <doodle_app/gui/base/base_window.h>

#include <string>

namespace doodle::gui {

class DOODLE_APP_API show_message {
  class impl;
  std::unique_ptr<impl> p_i;
  std::string Message;

 public:
  explicit show_message();
  explicit show_message(const std::string& in_msg);
  virtual ~show_message();

  std::int32_t flags() const;
  bool render();
  const std::string& title() const;
  show_message& set_message(const std::string& in_msg);  /// 设置消息内容
  std::string get_message();
};

}  // namespace doodle::gui
