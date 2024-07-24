//
// Created by TD on 2021/9/17.
//

#include "long_time_tasks_widget.h"

#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/metadata/metadata_cpp.h>

#include <doodle_app/lib_warp/imgui_warp.h>

#include <boost/range.hpp>
#include <boost/range/algorithm_ext.hpp>

#include <fmt/chrono.h>

namespace doodle::gui {
long_time_tasks_widget::long_time_tasks_widget() : p_current_select() { title_name_ = std::string{name}; }

bool long_time_tasks_widget::render() {
  static auto flags{
      ImGuiTableFlags_::ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_::ImGuiTableFlags_Resizable |
      ImGuiTableFlags_::ImGuiTableFlags_BordersOuter | ImGuiTableFlags_::ImGuiTableFlags_BordersV |
      ImGuiTableFlags_::ImGuiTableFlags_ContextMenuInBody
  };
  dear::Table{"long_time_tasks_widget", 6, flags, ImVec2{0, -350}} && [this]() {
    imgui::TableSetupColumn("名称");
    imgui::TableSetupColumn("进度");
    imgui::TableSetupColumn("消息");
    imgui::TableSetupColumn("状态");
    imgui::TableSetupColumn("时间");
    imgui::TableSetupColumn("动作");
    imgui::TableHeadersRow();

    for (const auto&& [e, msg] : g_reg()->view<process_message>().each()) {
      auto k_h = entt::handle{*g_reg(), e};
      imgui::TableNextRow();
      imgui::TableNextColumn();
      if (dear::Selectable(
        msg.get_name_id(), p_current_select == k_h,
        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap
      )) {
        p_current_select = k_h;
      }

      imgui::TableNextColumn();
      dear::Text(fmt::format("{:04f}%", msg.get_progress_f()));

      imgui::TableNextColumn();
      auto&& l_message_back = msg.message_back();
      imgui::TextUnformatted(l_message_back.data(), l_message_back.data() + l_message_back.size());

      imgui::TableNextColumn();
      //      dear::Text(std::string{magic_enum::enum_name(msg.get_state())});
      switch (msg.get_state()) {
        case process_message::state::wait:
          ImGui::Text("准备任务");
          break;
        case process_message::state::run:
          ImGui::Text("正在运行");
          break;
        case process_message::state::pause: {
          dear::WithStyleColor l_color{ImGuiCol_Text, ImVec4{1.0f, 1.0f, 0.0f, 1.0f}};
          ImGui::Text("暂停中...");
        }
        break;
        case process_message::state::fail: {
          dear::WithStyleColor l_color{ImGuiCol_Text, ImVec4{1.0f, 0.0f, 0.0f, 1.0f}};
          ImGui::Text("错误结束");
        }
        break;
        case process_message::state::success: {
          dear::WithStyleColor l_color{ImGuiCol_Text, ImVec4{0.0f, 1.0f, 0.0f, 1.0f}};
          ImGui::Text("成功完成");
        }
        break;
      }

      imgui::TableNextColumn();
      using namespace std::literals;
      dear::Text(
        msg.get_time() == chrono::sys_time_pos::duration{0} ? "..."s : fmt::format("{:%H:%M:%S}", msg.get_time())
      );

      ImGui::TableNextColumn();

      if (!msg.is_connected()) {
        if (ImGui::SmallButton(fmt::format("关闭##{}", msg.get_name_id()).c_str())) msg.aborted();
      } else
        ImGui::Text("无");
    }
  };
  ImGui::Combo(
    "日志等级", reinterpret_cast<int*>(&index_),
    [](void* in_data, std::int32_t in_index, const char** out_text) -> bool {
      constexpr auto l_leve_names_tmp = magic_enum::enum_names<level::level_enum>();
      *out_text                       = l_leve_names_tmp[in_index].data();
      return true;
    },
    nullptr, static_cast<std::int32_t>(magic_enum::enum_count<level::level_enum>()) - 2
  );

  dear::Text("主要日志"s);
  if (dear::Child l_c{"main_log", ImVec2{0, 266}, true}; l_c) {
    if (p_current_select && p_current_select.any_of<process_message>()) {
      auto&& l_text = p_current_select.get<process_message>().level_log(index_);
      dear::TextWrapPos l_wrap{};
      imgui::TextUnformatted(l_text.data());
    }
  }

  return open;
}

const std::string& long_time_tasks_widget::title() const { return title_name_; }
} // namespace doodle::gui