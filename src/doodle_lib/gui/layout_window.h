//
// Created by TD on 2022/9/29.
//
#include <doodle_app/doodle_app_fwd.h>
#include <doodle_app/gui/base/base_window.h>

namespace doodle {
namespace gui {

class DOODLELIB_API layout_window : public detail::layout_tick_interface {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  layout_window();
  virtual ~layout_window() override;

  bool tick();
};

}  // namespace gui
}  // namespace doodle

