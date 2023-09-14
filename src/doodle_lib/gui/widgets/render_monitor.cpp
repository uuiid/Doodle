//
// Created by td_main on 2023/8/22.
//

#include "render_monitor.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <doodle_app/lib_warp/imgui_warp.h>

#include <doodle_lib/render_farm/udp_client.h>

#include <winreg/WinReg.hpp>
namespace doodle {
namespace gui {
void render_monitor::init() {
  p_i->strand_ptr_       = std::make_shared<strand_t>(boost::asio::make_strand(g_io_context()));
  p_i->timer_ptr_        = std::make_shared<timer_t>(*p_i->strand_ptr_);

  p_i->progress_message_ = "正在查找服务器";
  p_i->logger_ptr_ = g_logger_ctrl().make_log(fmt::format("{} {}", typeid(render_monitor).name(), fmt::ptr(this)));
  do_find_server_address();
}
bool render_monitor::render() {
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
        ImGui::Text(l_computer.id_.c_str());
        ImGui::TableNextColumn();
        dear::Text(l_computer.computer_.name_);
        ImGui::TableNextColumn();
        dear::Text(l_computer.computer_.state_);
      }
    }
  }

  if (auto l_ = dear::CollapsingHeader{
          *p_i->render_task_collapsing_header_id_, ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen}) {
    if (auto l_table = dear::Table{*p_i->render_task_table_id_, 4}) {
      ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
      ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 100.0f);
      ImGui::TableSetupColumn("名称");
      ImGui::TableSetupColumn("状态");
      ImGui::TableSetupColumn("时间");

      ImGui::TableHeadersRow();

      for (auto& l_render_task : p_i->render_tasks_) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::Selectable(
                l_render_task.id_.c_str(), false,
                ImGuiSelectableFlags_::ImGuiSelectableFlags_SpanAllColumns |
                    ImGuiSelectableFlags_::ImGuiSelectableFlags_AllowDoubleClick
            )) {
          FSys::open_explorer(l_render_task.task_.path_);
        }
        ImGui::TableNextColumn();
        dear::Text(l_render_task.task_.name_);
        ImGui::TableNextColumn();
        dear::Text(l_render_task.task_.state_);
        ImGui::TableNextColumn();
        dear::Text(l_render_task.task_.time_);
      }
    }
  }

  return open_;
}
void render_monitor::do_find_server_address() {
  winreg::RegKey l_key{};
  l_key.Open(HKEY_CURRENT_USER, L"Software\\Doodle\\RenderFarm", KEY_READ | KEY_WOW64_64KEY);
  auto l_server_address = conv::utf_to_utf<char>(l_key.GetStringValue(L"server_address"));
  log_info(p_i->logger_ptr_, fmt::format("找到服务器 ip {}", l_server_address));
  p_i->client_ptr_ = std::make_shared<client>(l_server_address);
  do_wait();
}

void render_monitor::do_wait() {
  p_i->timer_ptr_->expires_after(doodle::chrono::seconds{3});
  p_i->timer_ptr_->async_wait([this, self = shared_from_this()](boost::system::error_code ec) {
    if (!open_) return;
    p_i->progress_ = 0.f;
    if (ec) {
      log_info(p_i->logger_ptr_, fmt::format("{}", ec));
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
      log_error(p_i->logger_ptr_, fmt::format("{}", in_code));
      p_i->progress_message_ = fmt::format("{}", in_code);
      return;
    }
    p_i->progress_message_.clear();

    p_i->computers_ = in_vector |
                      ranges::views::transform([](const auto& in_computer) { return computer_gui{in_computer}; }) |
                      ranges::to<std::vector<computer_gui>>();
  });
  p_i->client_ptr_->async_task_list([this, self = shared_from_this()](
                                        const boost::system::error_code& in_code,
                                        const std::vector<client::task_t>& in_vector
                                    ) {
    if (in_code) {
      log_error(p_i->logger_ptr_, fmt::format("{}", in_code));
      p_i->progress_message_ = fmt::format("{}", in_code);
      do_wait();
      return;
    }
    p_i->progress_message_.clear();
    p_i->render_tasks_ = in_vector | ranges::views::transform([](const auto& in_task) { return task_t_gui{in_task}; }) |
                         ranges::to<std::vector<task_t_gui>>();
    do_wait();
  });
}
render_monitor::~render_monitor() = default;
}  // namespace gui
}  // namespace doodle