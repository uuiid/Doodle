//
// Created by TD on 2021/9/17.
//

#include "long_time_tasks_widget.h"

#include <DoodleLib/Gui/action/command.h>
#include <DoodleLib/Metadata/Metadata_cpp.h>
#include <DoodleLib/core/DoodleLib.h>
#include <DoodleLib/libWarp/imgui_warp.h>
#include <DoodleLib/threadPool/long_term.h>
namespace doodle {

long_time_tasks_widget::long_time_tasks_widget()
    : task(),
      p_current_select(),
      p_command_tool_ptr_(),
      p_main_log(),
      p_info_log() {
  //  for (int k_i = 0; k_i < 5000; ++k_i) {
  //    p_main_log.p_log.append("p_main_log test\n");
  //    p_info_log.p_log.append("p_info_log test\n");
  //  }
}

void long_time_tasks_widget::frame_render() {
  auto& k_ = DoodleLib::Get();
  {
    std::lock_guard k_guard{k_.mutex};
    task = k_.long_task_list;
  }
  if (p_command_tool_ptr_)
    p_command_tool_ptr_->run();

  dear::Table{"long_time_tasks_widget", 5} && [this]() {
    imgui::TableSetupColumn("名称");
    imgui::TableSetupColumn("进度");
    imgui::TableSetupColumn("消息");
    imgui::TableSetupColumn("状态");
    imgui::TableSetupColumn("时间");
    imgui::TableHeadersRow();

    for (const auto& i : task) {
      imgui::TableNextRow();
      imgui::TableNextColumn();
      if (dear::Selectable(i->get_id(),
                           p_current_select == i,
                           ImGuiSelectableFlags_SpanAllColumns)) {
        p_current_select = i;
        p_main_log       = main_log{};
        p_info_log       = info_log{};
        link_main_log();
        link_info_log();
      }

      imgui::TableNextColumn();
      dear::Text(fmt::format("{:04f}%", i->get_progress_int()));

      imgui::TableNextColumn();
      dear::Text(i->message_result());

      imgui::TableNextColumn();
      dear::Text(i->get_state_str().data());

      imgui::TableNextColumn();
      dear::Text(i->get_time_str());
    }
  };
  dear::Text("主要日志");
  dear::Child{"main_log", ImVec2{0, 266}, true} && [this]() {
    imgui::TextUnformatted(p_main_log.p_log.begin(), p_main_log.p_log.end());
  };
  dear::Text("全部信息");
  dear::Child{"info_log", ImVec2{0, 266}, true} && [this]() {
    imgui::TextUnformatted(p_info_log.p_log.begin(), p_info_log.p_log.end());
  };
}
void long_time_tasks_widget::push_back(const long_term_ptr& in_term) {
  task.push_back(in_term);
}
void long_time_tasks_widget::set_tool_widget(const command_tool_ptr& in_ptr) {
  p_command_tool_ptr_ = in_ptr;
}
void long_time_tasks_widget::link_main_log() {
  for (const auto& i : p_current_select->message()) {
    p_main_log.p_log.append(i.data(), i.data() + i.size());
  }
  p_main_log.p_conn_list.emplace_back(
      boost::signals2::scoped_connection{
          p_current_select->sig_message_result.connect(
              [this](const std::string& in_string, long_term::level in_level) {
                if (in_level == long_term::warning)
                  p_main_log.p_log.append(in_string.data(),
                                          in_string.data() + in_string.size());
              })});
}
void long_time_tasks_widget::link_info_log() {
  for (const auto& i : p_current_select->log()) {
    p_info_log.p_log.append(i.data(), i.data() + i.size());
  }
  p_info_log.p_conn_list.emplace_back(
      boost::signals2::scoped_connection{
          p_current_select->sig_message_result.connect(
              [this](const std::string& in_string, long_term::level in_level) {
                p_info_log.p_log.append(in_string.data(),
                                        in_string.data() + in_string.size());
              })});
}
}  // namespace doodle
