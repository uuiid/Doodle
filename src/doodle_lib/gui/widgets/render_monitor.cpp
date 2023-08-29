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
  if (ImGui::Button("ull")) {
    std::int32_t* l_id{nullptr};
    throw doodle_error{""};
  }

  std::call_once(p_i->once_flag_, [this]() { init(); });
  {
    ImGui::Text("渲染刷新");
    ImGui ::SameLine();
    p_i->progress_ += (1.0f / (3.0f * 60.0f));

    ImGui::ProgressBar(p_i->progress_, ImVec2{200.f, 0.0f});
    if (!p_i->progress_message_.empty()) {
      ImGui::SameLine();
      dear::Text(p_i->progress_message_);
    }
  }
  if (auto l_ = dear::CollapsingHeader{
          *p_i->component_collapsing_header_id_, ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen}) {
    if (auto l_table = dear::Table{*p_i->component_table_id_, 3}) {
      ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
      ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 100.0f);
      ImGui::TableSetupColumn("名称");
      ImGui::TableSetupColumn("状态");

      ImGui::TableHeadersRow();
      for (auto& l_computer : p_i->computers_) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%d", l_computer.id_);
        ImGui::TableNextColumn();
        dear::Text(l_computer.name_);
        ImGui::TableNextColumn();
        dear::Text(l_computer.state_);
      }
    }
  }

  if (auto l_ = dear::CollapsingHeader{
          *p_i->render_task_collapsing_header_id_, ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen}) {
    if (auto l_table = dear::Table{*p_i->render_task_table_id_, 3}) {
      ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
      ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 100.0f);
      ImGui::TableSetupColumn("名称");
      ImGui::TableSetupColumn("状态");

      ImGui::TableHeadersRow();

      for (auto& l_render_task : p_i->render_tasks_) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%d", l_render_task.id_);
        ImGui::TableNextColumn();
        dear::Text(l_render_task.name_);
        ImGui::TableNextColumn();
        dear::Text(l_render_task.state_);
      }
    }
  }

  return open_;
}
void render_monitor::do_wait() {
  p_i->timer_ptr_->expires_after(doodle::chrono::seconds{3});
  p_i->timer_ptr_->async_wait([this, self = shared_from_this()](boost::system::error_code ec) {
    if (!open_) return;
    p_i->progress_ = 0.f;
    if (ec) {
      DOODLE_LOG_ERROR("{}", ec);
      return;
    }
    get_remote_data();
  });
}
void render_monitor::get_remote_data() {
  p_i->client_ptr_->async_computer_list([this, self = shared_from_this()](
                                            const boost::system::error_code& in_code,
                                            const std::vector<client::computer>& in_vector
                                        ) {
    if (in_code) {
      DOODLE_LOG_ERROR("{}", in_code);
      p_i->progress_message_ = fmt::format("{}", in_code);
      do_wait();
      return;
    }
    p_i->progress_message_.clear();

    p_i->computers_ = in_vector | ranges::views::transform([](auto&& in_item) -> computer {
                        return computer{in_item.id_, in_item.name_, in_item.state_};
                      }) |
                      ranges::to_vector;
    do_wait();
  });
  p_i->client_ptr_->async_task_list([this, self = shared_from_this()](
                                        const boost::system::error_code& in_code,
                                        const std::vector<client::task_t>& in_vector
                                    ) {
    if (in_code) {
      DOODLE_LOG_ERROR("{}", in_code);
      p_i->progress_message_ = fmt::format("{}", in_code);
      do_wait();
      return;
    }
    p_i->progress_message_.clear();
    p_i->render_tasks_ = in_vector | ranges::views::transform([](auto&& in_item) -> render_task {
                           return render_task{in_item.id_, in_item.name_, in_item.state_};
                         }) |
                         ranges::to_vector;
    do_wait();
  });
}
render_monitor::~render_monitor() = default;
}  // namespace gui
}  // namespace doodle