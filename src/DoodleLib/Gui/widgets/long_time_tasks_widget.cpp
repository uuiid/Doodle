//
// Created by TD on 2021/9/17.
//

#include "long_time_tasks_widget.h"

#include <DoodleLib/Metadata/Metadata_cpp.h>
#include <DoodleLib/core/DoodleLib.h>
#include <DoodleLib/libWarp/imgui_warp.h>
#include <DoodleLib/threadPool/long_term.h>

namespace doodle {

long_time_tasks_widget::long_time_tasks_widget()
    : task() {
}

void long_time_tasks_widget::frame_render() {
  auto& k_ = DoodleLib::Get();
  {
    std::lock_guard k_guard{k_.mutex};
    task = k_.long_task_list;
  }
  dear::Table{"long_time_tasks_widget", 2} && [this]() {
    imgui::TableSetupColumn("名称");
    imgui::TableSetupColumn("进度");
    imgui::TableSetupColumn("消息");
    imgui::TableSetupColumn("状态");
    imgui::TableSetupColumn("时间");
    imgui::TableHeadersRow();

    for (const auto& i : task) {
      imgui::TableNextRow();
      imgui::TableNextColumn();
      if (dear::Selectable(i->get_id(), p_current_select == i)) {
        p_current_select = i;
      }

      imgui::TableNextColumn();
    }
  };
}
void long_time_tasks_widget::push_back(const long_term_ptr& in_term) {
  task.push_back(in_term);
}
}  // namespace doodle
