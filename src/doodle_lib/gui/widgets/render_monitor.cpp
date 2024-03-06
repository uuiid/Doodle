//
// Created by td_main on 2023/8/22.
//

#include "render_monitor.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/core/http_client_core.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_app/lib_warp/imgui_warp.h>

#include <doodle_lib/core/http/json_body.h>
namespace doodle {
namespace gui {
void render_monitor::init() {
  p_i->strand_ptr_       = std::make_shared<strand_t>(boost::asio::make_strand(g_io_context()));
  p_i->timer_ptr_        = std::make_shared<timer_t>(*p_i->strand_ptr_);

  p_i->progress_message_ = "正在查找服务器";
  p_i->logger_ptr_       = g_logger_ctrl().make_log("render_monitor");
  do_find_server_address();
}
bool render_monitor::render() {
  std::call_once(p_i->once_flag_, [this]() { init(); });

  {
    ImGui::Text("渲染刷新");
    ImGui ::SameLine();
    if (p_i->progress_ < 1.0f) p_i->progress_ += (1.0f / (3.0f * 60.0f));

    ImGui::ProgressBar(p_i->progress_, ImVec2{200.f, 0.0f});
    if (!p_i->progress_message_.empty()) {
      ImGui::SameLine();
      dear::Text(p_i->progress_message_);
      if (p_i->progress_ >= 1.0f) {
        p_i->progress_ = 0.f;
      }
    }
  }
  if (auto l_ = dear::CollapsingHeader{
          *p_i->component_collapsing_header_id_, ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen
      }) {
    if (auto l_table = dear::Table{*p_i->component_table_id_, 4}) {
      ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
      ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 100.0f);
      ImGui::TableSetupColumn("名称");
      ImGui::TableSetupColumn("ip");
      ImGui::TableSetupColumn("状态");

      ImGui::TableHeadersRow();
      for (auto& l_computer : p_i->computers_) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text(l_computer.id_.c_str());
        ImGui::TableNextColumn();
        dear::Text(l_computer.name_);
        ImGui::TableNextColumn();
        dear::Text(l_computer.ip_);
        ImGui::TableNextColumn();
        dear::Text(l_computer.status_);
      }
    }
  }

  if (auto l_ = dear::CollapsingHeader{
          *p_i->render_task_collapsing_header_id_, ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen
      }) {
    if (auto l_table = dear::Table{*p_i->render_task_table_id_, 10}) {
      ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
      ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 100.0f);
      ImGui::TableSetupColumn("名称");
      ImGui::TableSetupColumn("状态");

      ImGui::TableSetupColumn("提交计算机");
      ImGui::TableSetupColumn("提交者");
      ImGui::TableSetupColumn("提交时间");

      ImGui::TableSetupColumn("运行计算机");
      ImGui::TableSetupColumn("运行计算机ip");
      ImGui::TableSetupColumn("运行时间");
      ImGui::TableSetupColumn("结束时间");
      ImGui::TableHeadersRow();

      for (auto& l_render_task : p_i->render_tasks_) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::Selectable(
                l_render_task.id_.c_str(), false,
                ImGuiSelectableFlags_::ImGuiSelectableFlags_SpanAllColumns |
                    ImGuiSelectableFlags_::ImGuiSelectableFlags_AllowDoubleClick
            )) {
          FSys::open_explorer(l_render_task.id_);
        }
        ImGui::TableNextColumn();
        dear::Text(l_render_task.name_);
        ImGui::TableNextColumn();
        dear::Text(l_render_task.status_);
        ImGui::TableNextColumn();
        dear::Text(l_render_task.source_computer_);
        ImGui::TableNextColumn();
        dear::Text(l_render_task.submitter_);
        ImGui::TableNextColumn();
        dear::Text(l_render_task.submit_time_);
        ImGui::TableNextColumn();
        dear::Text(l_render_task.run_computer_);
        ImGui::TableNextColumn();
        dear::Text(l_render_task.run_computer_ip_);
        ImGui::TableNextColumn();
        dear::Text(l_render_task.run_time_);
        ImGui::TableNextColumn();
        dear::Text(l_render_task.end_time_);
      }
    }
  }

  return open_;
}
void render_monitor::do_find_server_address() {
  p_i->http_client_core_ptr_ =
      std::make_shared<http::detail::http_client_core>(register_file_type::get_server_address());
  do_wait();
}

void render_monitor::do_wait() {
  p_i->timer_ptr_->expires_after(doodle::chrono::seconds{3});
  p_i->timer_ptr_->async_wait(boost::asio::bind_cancellation_slot(
      app_base::GetPtr()->on_cancel.slot(),
      [this, self = shared_from_this()](boost::system::error_code ec) {
        if (!open_) return;
        p_i->progress_ = 0.f;
        if (ec) {
          log_info(p_i->logger_ptr_, fmt::format("{}", ec));
          return;
        }
        get_remote_data();
      }
  ));
}
void render_monitor::get_remote_data() {
  // get computers
  boost::beast::http::request<boost::beast::http::empty_body> l_req_computer{
      boost::beast::http::verb::get, "v1/computer", 11
  };
  l_req_computer.keep_alive(true);
  l_req_computer.set(boost::beast::http::field::accept, "application/json");

  p_i->http_client_core_ptr_->async_read<boost::beast::http::response<http::basic_json_body>>(
      l_req_computer, boost::asio::bind_executor(
                          g_io_context(), boost::asio::bind_cancellation_slot(
                                              app_base::GetPtr()->on_cancel.slot(),
                                              [this, self = shared_from_this()](
                                                  const boost::system::error_code& in_code,
                                                  const boost::beast::http::response<http::basic_json_body>& in_vector
                                              ) {
                                                if (in_code) {
                                                  log_error(p_i->logger_ptr_, fmt::format("{}", in_code));
                                                  p_i->progress_message_ = fmt::format("{}", in_code);
                                                  return;
                                                }
                                                p_i->progress_message_.clear();
                                              }
                                          )
                      )
  );
  boost::beast::http::request<boost::beast::http::empty_body> l_req_task{boost::beast::http::verb::get, "v1/task", 11};
  l_req_task.keep_alive(true);
  l_req_task.set(boost::beast::http::field::accept, "application/json");

  p_i->http_client_core_ptr_->async_read<boost::beast::http::response<http::basic_json_body>>(
      l_req_task, boost::asio::bind_executor(
                      g_io_context(), boost::asio::bind_cancellation_slot(
                                          app_base::GetPtr()->on_cancel.slot(),
                                          [this, self = shared_from_this()](
                                              const boost::system::error_code& in_code,
                                              const boost::beast::http::response<http::basic_json_body>& in_vector
                                          ) {
                                            if (in_code) {
                                              log_error(p_i->logger_ptr_, fmt::format("{}", in_code));
                                              p_i->progress_message_ = fmt::format("{}", in_code);
                                              do_wait();
                                              return;
                                            }
                                            p_i->progress_message_.clear();
                                            do_wait();
                                          }
                                      )
                  )
  );
}
render_monitor::~render_monitor() = default;
}  // namespace gui
}  // namespace doodle