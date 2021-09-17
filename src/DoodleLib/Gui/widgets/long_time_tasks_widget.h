//
// Created by TD on 2021/9/17.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/base_windwos.h>

#include <boost/signals2.hpp>

namespace doodle {

class DOODLELIB_API long_time_tasks_widget : public base_widget {
  std::vector<long_term_ptr> task;
  long_term_ptr p_current_select;
 public:
  long_time_tasks_widget();
  void push_back(const long_term_ptr& in_term);

  virtual void frame_render() override;
};
}  // namespace doodle
