//
// Created by TD on 2021/9/17.
//

#include "long_time_tasks_widget.h"

#include <doodle_core/core/doodle_lib.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_core/metadata/metadata_cpp.h>

#include <boost/range.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <fmt/chrono.h>
#include <doodle_core/core/init_register.h>

namespace doodle {

long_time_tasks_widget::long_time_tasks_widget()
    : p_current_select() {
  title_name_ = std::string{name};
}

void long_time_tasks_widget::render() {
  static auto flags{ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_::ImGuiTableFlags_Resizable | ImGuiTableFlags_::ImGuiTableFlags_BordersOuter | ImGuiTableFlags_::ImGuiTableFlags_BordersV | ImGuiTableFlags_::ImGuiTableFlags_ContextMenuInBody};
  dear::Table{"long_time_tasks_widget", 6, flags} && [this]() {
    imgui::TableSetupColumn("名称");
    imgui::TableSetupColumn("进度");
    imgui::TableSetupColumn("消息");
    imgui::TableSetupColumn("状态");
    imgui::TableSetupColumn("时间");
    imgui::TableSetupColumn("动作");
    imgui::TableHeadersRow();

    for (const auto&& [e, msg] : g_reg()->view<process_message>().each()) {
      auto k_h = make_handle(e);
      imgui::TableNextRow();
      imgui::TableNextColumn();
      if (dear::Selectable(msg.get_name_id(), p_current_select == k_h, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
        p_current_select = k_h;
      }

      imgui::TableNextColumn();
      dear::Text(fmt::format("{:04f}%", msg.get_progress_f()));

      imgui::TableNextColumn();
      dear::Text(msg.message_back());

      imgui::TableNextColumn();
      dear::Text(std::string{magic_enum::enum_name(msg.get_state())});

      imgui::TableNextColumn();
      using namespace std::literals;
      dear::Text(msg.is_wait() ? "..."s : fmt::format("{:%H:%M:%S}", msg.get_time()));

      ImGui::TableNextColumn();
      if (ImGui::Button(fmt::format("关闭##{}", msg.get_name_id()).c_str()))
        msg.aborted_function();
    }
  };
  dear::Text("主要日志"s);
  dear::Child{"main_log", ImVec2{0, 266}, true} && [this]() {
    if (p_current_select && p_current_select.any_of<process_message>()) {
      auto msg_str = p_current_select.get<process_message>().err();
      imgui::TextUnformatted(msg_str.data(), msg_str.data() + msg_str.size());
    }
    // dear::TextWrapPos{0.0f} && [this]() {
    // };
  };
  dear::Text("全部信息"s);
  dear::Child{"info_log", ImVec2{0, 266}, true} && [this]() {
    if (p_current_select && p_current_select.any_of<process_message>()) {
      auto msg_str = p_current_select.get<process_message>().log();
      imgui::TextUnformatted(msg_str.data(), msg_str.data() + msg_str.size());
    }
  };
}
}  // namespace doodle
