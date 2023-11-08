//
// Created by td_main on 2023/11/7.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_app/doodle_app_fwd.h>
#include <doodle_app/gui/base/ref_base.h>

namespace doodle::gui {
class open_project {
  bool open{true};
  std::shared_ptr<authorization> auth_ptr_;
  // 到期时间
  std::string expire_time_str_;
  // 授权码
  gui_cache<std::string> auth_code_{"授权码"};

 public:
  open_project();
  ~open_project() = default;
  constexpr static std::string_view name{"初始化"};
  static constexpr std::array<float, 2> sizexy{640, 360};
  bool render();
};

}  // namespace doodle::gui
