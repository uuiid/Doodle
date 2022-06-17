//
// Created by TD on 2022/6/17.
//

#include <doodle_lib/doodle_lib_fwd.h>

#include <doodle_lib/gui/gui_ref/base_window.h>
namespace doodle::gui {

class DOODLELIB_API main_windows : public window_panel {
  class impl;
  std::unique_ptr<impl> pi_;
 public:
  main_windows();
  virtual ~main_windows();
  virtual void init() override;

 protected:
  virtual void render() override;
};

}  // namespace doodle::gui
