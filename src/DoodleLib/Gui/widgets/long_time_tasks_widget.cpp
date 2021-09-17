//
// Created by TD on 2021/9/17.
//

#include "long_time_tasks_widget.h"

#include <DoodleLib/Metadata/Metadata_cpp.h>
#include <DoodleLib/libWarp/imgui_warp.h>
namespace doodle {

long_time_tasks_widget::long_time_tasks_widget()
    : task() {
}

void long_time_tasks_widget::frame_render() {
}
void long_time_tasks_widget::push_back(const long_term_ptr& in_term) {
  task.push_back(in_term);
}
}  // namespace doodle
