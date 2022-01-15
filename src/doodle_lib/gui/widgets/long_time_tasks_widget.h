//
// Created by TD on 2021/9/17.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>
#include <doodle_lib/lib_warp/imgui_warp.h>

#include <boost/signals2.hpp>
namespace doodle {

class DOODLELIB_API long_time_tasks_widget : public process_t<long_time_tasks_widget> {
  entt::handle p_current_select;

 public:
  long_time_tasks_widget();


  constexpr static std::string_view name{"队列"};

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(delta_type, void* data);
};
}  // namespace doodle
