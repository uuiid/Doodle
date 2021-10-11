//
// Created by TD on 2021/9/16.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>

#include <boost/signals2.hpp>
namespace doodle {

class DOODLELIB_API project_widget : public metadata_widget {
 public:
  project_widget();
  void frame_render() override;

  project_ptr p_current_select;
  
  boost::signals2::signal<void(const project_ptr&)> select_change;
};
}  // namespace doodle
