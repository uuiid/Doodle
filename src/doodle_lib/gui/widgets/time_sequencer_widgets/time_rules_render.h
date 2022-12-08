//
// Created by TD on 2022/8/4.
//

#pragma once

#include <doodle_app/gui/base/modify_guard.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <iterator>
#include <string>

namespace doodle {
namespace business {
class rules;
}

namespace gui::time_sequencer_widget_ns {

class DOODLELIB_API time_rules_render {
 public:
  using rules_type = ::doodle::business::rules;

 private:
  class impl;
  std::unique_ptr<impl> p_i;

  void print_show_str();
  std::string get_work_str(std::size_t in_index);

 public:
  modify_guard modify_guard_{};

  time_rules_render();
  virtual ~time_rules_render();

  [[nodiscard]] const rules_type& rules_attr() const;
  void rules_attr(const rules_type& in_rules_type);
  bool render();
};

}  // namespace gui::time_sequencer_widget_ns
}  // namespace doodle
