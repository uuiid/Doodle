//
// Created by TD on 2022/9/21.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle::gui::detail {

class DOODLE_CORE_API windows_tick_interface {
 public:
  virtual ~windows_tick_interface() = default;
  /**
   * 当 tick 返回 true 时, 会将其在定时器中弹出并销毁
   */
  virtual bool tick()               = 0;
};

class DOODLE_CORE_API windows_render_interface
    : public windows_tick_interface {
 public:
  virtual ~windows_render_interface() override                                   = default;
  [[nodiscard("Back to Window Title")]] virtual const std::string& title() const = 0;
};

class DOODLE_CORE_API layout_tick_interface
    : public windows_tick_interface {
 public:
};

using windows_tick   = std::shared_ptr<windows_tick_interface>;
using windows_render = std::shared_ptr<windows_render_interface>;
using layout_tick    = std::shared_ptr<layout_tick_interface>;

}  // namespace doodle::gui::detail
