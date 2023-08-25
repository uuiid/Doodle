//
// Created by td_main on 2023/8/22.
//

#include "render_monitor.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <doodle_app/lib_warp/imgui_warp.h>
namespace doodle {
namespace gui {
void render_monitor::init() {
  p_i->client_ptr_ = std::make_shared<client>(core_set::get_set().server_ip);
  p_i->strand_ptr_ = std::make_shared<strand_t>(boost::asio::make_strand(g_io_context()));
  p_i->timer_ptr_  = std::make_shared<timer_t>(*p_i->strand_ptr_);

  do_wait();
}
bool render_monitor::render() {
  std::call_once(p_i->once_flag_, [this]() { init(); });
  if (auto l_ = dear::CollapsingHeader{*p_i->component_collapsing_header_id_}) {
    if (auto l_table = dear::Table{*p_i->component_table_id_, 3}) {
      ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
      ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 100.0f);
      ImGui::TableSetupColumn("名称");
      ImGui::TableSetupColumn("状态");

      ImGui::TableHeadersRow();
      for (auto& l_computer : p_i->computers_) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%ll", l_computer.id_);
        ImGui::TableNextColumn();
        dear::Text(l_computer.name_);
        ImGui::TableNextColumn();
        dear::Text(l_computer.state_);
      }
    }
  }

  if (auto l_ = dear::CollapsingHeader{*p_i->render_task_collapsing_header_id_}) {
    if (auto l_table = dear::Table{*p_i->render_task_table_id_, 3}) {
      ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
      ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 100.0f);
      ImGui::TableSetupColumn("名称");
      ImGui::TableSetupColumn("状态");

      ImGui::TableHeadersRow();

      for (auto& l_render_task : p_i->render_tasks_) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%ll", l_render_task.id_);
        ImGui::TableNextColumn();
        dear::Text(l_render_task.name_);
        ImGui::TableNextColumn();
        dear::Text(l_render_task.state_);
      }
    }
  }

  return p_i->open_;
}
void render_monitor::do_wait() {
  p_i->timer_ptr_->expires_after(doodle::chrono::seconds{3});
  p_i->timer_ptr_->async_wait([this](boost::system::error_code ec) {
    if (ec) {
      DOODLE_LOG_ERROR("{}", ec);
      return;
    }
    get_remote_data();
  });
}
void render_monitor::get_remote_data() {
  p_i->client_ptr_->async_computer_list(
      [this](const boost::system::error_code& in_code, const std::vector<client::computer>& in_vector) {
        if (in_code) {
          DOODLE_LOG_ERROR("{}", in_code);
          do_wait();
          return;
        }
        p_i->computers_ = in_vector | ranges::views::transform([](auto&& in_item) -> computer {
                            return computer{in_item.id_, in_item.name_, in_item.state_};
                          }) |
                          ranges::to_vector;
        do_wait();
      }
  );
  p_i->client_ptr_->async_task_list(
      [this](const boost::system::error_code& in_code, const std::vector<client::task_t>& in_vector) {
        if (in_code) {
          DOODLE_LOG_ERROR("{}", in_code);
          do_wait();
          return;
        }
        p_i->render_tasks_ = in_vector | ranges::views::transform([](auto&& in_item) -> render_task {
                               return render_task{in_item.id_, in_item.name_, in_item.state_};
                             }) |
                             ranges::to_vector;
        do_wait();
      }
  );
}
}  // namespace gui
}  // namespace doodle