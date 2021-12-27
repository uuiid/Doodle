//
// Created by TD on 2021/9/17.
//

#include "long_time_tasks_widget.h"

#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata_cpp.h>
#include <doodle_lib/thread_pool/long_term.h>

#include <boost/range.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <fmt/chrono.h>

namespace doodle {

long_time_tasks_widget::long_time_tasks_widget()
    : task(),
      p_current_select(),
      p_command_tool_ptr_(),
      p_main_log(),
      p_info_log() {
  p_class_name = "队列";
  //  for (int k_i = 0; k_i < 5000; ++k_i) {
  //    p_main_log.p_log.append("p_main_log test\n");
  //    p_info_log.p_log.append("p_info_log test\n");
  //  }
}

void long_time_tasks_widget::frame_render() {
  static auto flags{ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit |
                    ImGuiTableFlags_::ImGuiTableFlags_Resizable |
                    ImGuiTableFlags_::ImGuiTableFlags_BordersOuter |
                    ImGuiTableFlags_::ImGuiTableFlags_BordersV |
                    ImGuiTableFlags_::ImGuiTableFlags_ContextMenuInBody};
  dear::Table{"long_time_tasks_widget", 5, flags} && [this]() {
    imgui::TableSetupColumn("名称");
    imgui::TableSetupColumn("进度");
    imgui::TableSetupColumn("消息");
    imgui::TableSetupColumn("状态");
    imgui::TableSetupColumn("时间");
    imgui::TableHeadersRow();

    for (const auto&& [e, msg] : g_reg()->view<process_message>().each()) {
      auto k_h = make_handle(e);
      imgui::TableNextRow();
      imgui::TableNextColumn();
      if (dear::Selectable(msg.get_name(),
                           p_current_select == k_h,
                           ImGuiSelectableFlags_SpanAllColumns)) {
        p_current_select = k_h;
      }

      imgui::TableNextColumn();
      dear::Text(fmt::format("{:04f}%", msg.get_progress_f()));

      imgui::TableNextColumn();
      dear::Text(msg.message_back());

      imgui::TableNextColumn();
      dear::Text(string{magic_enum::enum_name(msg.get_state())});

      imgui::TableNextColumn();
      using namespace std::literals;
      dear::Text(msg.is_wait() ? "..."s : fmt::format("{}", msg.get_time()));
    }
  };
  dear::Text("主要日志");
  dear::Child{"main_log", ImVec2{0, 266}, true} && [this]() {
    if (p_current_select && p_current_select.any_of<process_message>()) {
      auto msg_str = p_current_select.get<process_message>().err();
      imgui::TextUnformatted(msg_str.data(), msg_str.data() + msg_str.size());
    }
    // dear::TextWrapPos{0.0f} && [this]() {
    // };
  };
  dear::Text("全部信息");
  dear::Child{"info_log", ImVec2{0, 266}, true} && [this]() {
    if (p_current_select && p_current_select.any_of<process_message>()) {
      auto msg_str = p_current_select.get<process_message>().log();
      imgui::TextUnformatted(msg_str.data(), msg_str.data() + msg_str.size());
    }
  };
}
void long_time_tasks_widget::push_back(const long_term_ptr& in_term) {
  task.push_back(in_term);
}
void long_time_tasks_widget::set_tool_widget(const command_ptr& in_ptr) {
  p_command_tool_ptr_ = in_ptr;
}
void long_time_tasks_widget::link_main_log() {
}
void long_time_tasks_widget::link_info_log() {
}
}  // namespace doodle
