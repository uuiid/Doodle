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

#include <boost/url.hpp>

namespace doodle {
namespace gui {
void render_monitor::task_t_gui::parse_time() {
  if (!run_time_.empty()) {
    std::istringstream l_ss{run_time_};
    l_ss >> chrono::parse("%Y-%m-%d %H:%M:%S", start_time_point_);
  }
  if (!end_time_.empty()) {
    std::istringstream l_ss{end_time_};
    l_ss >> chrono::parse("%Y-%m-%d %H:%M:%S", end_time_point_);
  }
}

void render_monitor::task_t_gui::update_duration() {
  if (start_time_point_.time_since_epoch().count() == 0) return;
  if (end_time_point_.time_since_epoch().count() == 0) {
    auto l_duration = decltype(start_time_point_)::clock::now() - start_time_point_;
    duration_       = fmt::format("{:%H:%M:%S}", l_duration);
    return;
  }
  auto l_duration = end_time_point_ - start_time_point_;
  duration_       = fmt::format("{:%H:%M:%S}", l_duration);
}

void render_monitor::init() {
  p_i->strand_ptr_       = std::make_shared<strand_t>(boost::asio::make_strand(g_io_context()));
  p_i->timer_ptr_        = std::make_shared<timer_t>(*p_i->strand_ptr_);

  p_i->progress_message_ = "正在查找服务器";
  p_i->logger_ptr_       = g_logger_ctrl().make_log("render_monitor");
  p_i->http_client_ptr_  = std::make_shared<client_t>(core_set::get_render_url());
  boost::asio::co_spawn(
      *p_i->strand_ptr_, async_refresh(),
      boost::asio::bind_cancellation_slot(p_i->signal_.slot(), boost::asio::detached)
  );
}

bool render_monitor::render() {
  std::call_once(p_i->once_flag_, [this]() { init(); });

  if (!p_i->progress_message_.empty()) {
    ImGui::SameLine();
    dear::Text(p_i->progress_message_);
  }
  if (auto l_ = dear::CollapsingHeader{
          *p_i->component_collapsing_header_id_, ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen
      }) {
    if (auto l_table = dear::Table{*p_i->component_table_id_, 3}) {
      ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible

      ImGui::TableSetupColumn("名称");
      ImGui::TableSetupColumn("ip");
      ImGui::TableSetupColumn("状态");

      ImGui::TableHeadersRow();
      for (auto& l_computer : p_i->computers_) {
        ImGui::TableNextRow();

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
    if (auto l_table = dear::Table{*p_i->render_task_table_id_, 11}) {
      ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible

      ImGui::TableSetupColumn("id");
      ImGui::TableSetupColumn("名称");
      ImGui::TableSetupColumn("状态");

      ImGui::TableSetupColumn("提交计算机");
      ImGui::TableSetupColumn("提交者");
      ImGui::TableSetupColumn("提交时间");

      ImGui::TableSetupColumn("运行计算机");
      ImGui::TableSetupColumn("运行计算机ip");
      ImGui::TableSetupColumn("运行时间");
      ImGui::TableSetupColumn("运行时间");
      ImGui::TableSetupColumn("动作");
      ImGui::TableHeadersRow();

      for (auto& l_render_task : p_i->render_tasks_) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        // 设置选择任务
        if (ImGui::Selectable(
                l_render_task.id_str_.c_str(),
                p_i->current_select_logger_ ? *p_i->current_select_logger_ == l_render_task.id_ : false
            )) {
          p_i->current_select_logger_ = l_render_task.id_;
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
        l_render_task.update_duration();
        dear::Text(l_render_task.duration_);

        ImGui::TableNextColumn();
        if (ImGui::Button(l_render_task.delete_button_id_.c_str())) {
          boost::asio::co_spawn(
              *p_i->strand_ptr_, async_delete_task(l_render_task.id_),
              boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached)
          );
        }
      }
    }
  }
  if (auto l_ = dear::CollapsingHeader{*p_i->logger_collapsing_header_id_}) {
    if (auto l_combo = dear::Combo{*p_i->logger_level_, p_i->logger_level_.data.c_str()}; l_combo) {
      for (auto&& i : magic_enum::enum_names<level::level_enum>()) {
        if (ImGui::Selectable(i.data(), p_i->index_ == magic_enum::enum_cast<level::level_enum>(i).value())) {
          p_i->index_             = magic_enum::enum_cast<level::level_enum>(i).value();
          p_i->logger_level_.data = i.data();
        }
      }
    }
    if (dear::Child l_c{*p_i->logger_child_id_, ImVec2{0, 266}, true}; l_c) {
      dear::TextWrapPos l_wrap{};
      imgui::TextUnformatted(p_i->logger_data.data(), p_i->logger_data.data() + p_i->logger_data.size());
    }
  }

  return open_;
}

boost::asio::awaitable<void> render_monitor::async_refresh() {
  auto l_self = p_i;

  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    auto l_c = co_await l_self->http_client_ptr_->get_computers();
    l_self->computers_.clear();
    for (auto& l_computer : l_c) {
      l_self->computers_.push_back(computer_gui{
          .name_   = l_computer.name_,
          .ip_     = l_computer.ip_,
          .status_ = std::string{magic_enum::enum_name(l_computer.client_status_)}
      });
    }
    co_await async_refresh_task();
    l_self->timer_ptr_->expires_after(3s);
    auto [l_ec] = co_await l_self->timer_ptr_->async_wait();
    if (l_ec) {
      log_info(l_self->logger_ptr_, fmt::format("{}", l_ec));
      l_self->progress_message_ = "获取数据失败";
    }
  }
}
boost::asio::awaitable<void> render_monitor::async_refresh_task() {
  auto l_self            = p_i;
  auto [l_tasks, l_size] = co_await l_self->http_client_ptr_->get_task(l_self->page_index_ * 50, 50);
  l_self->render_tasks_.clear();
  l_self->max_page_num_ = l_size / 50 + (l_size % 50 == 0 ? 0 : 1);
  std::size_t l_index{};
  for (auto& l_task : l_tasks) {
    l_self->render_tasks_.push_back(task_t_gui{
        .id_               = l_task.id_,
        .id_str_           = fmt::to_string(l_task.id_),
        .name_             = l_task.name_,
        .status_           = conv_state((l_task.status_)),
        .source_computer_  = l_task.source_computer_,
        .submitter_        = l_task.submitter_,
        .submit_time_      = fmt::to_string(l_task.submit_time_),
        .run_computer_     = l_task.run_computer_,
        .run_computer_ip_  = l_task.run_computer_ip_,
        .run_time_         = fmt::to_string(l_task.run_time_),
        .delete_button_id_ = fmt::format("删除任务##", ++l_index)
    });
  }
}

boost::asio::awaitable<void> render_monitor::async_delete_task(const uuid in_id) {
  auto l_self = p_i;
  co_await l_self->http_client_ptr_->delete_task(in_id);
  co_await async_refresh();
}

std::string render_monitor::conv_time(const nlohmann::json& in_json) {
  if (in_json.is_null()) return {};
  if (!in_json.is_string()) return {};
  auto l_time_str = in_json.get<std::string>();
  if (l_time_str.starts_with("1970")) return {};
  return l_time_str;
}

std::string render_monitor::conv_state(const nlohmann::json& in_json) {
  if (in_json.is_null()) return {};
  if (!in_json.is_string()) return {};
  static const std::pair<server_task_info_status, std::string> m[] = {
      {server_task_info_status::submitted, "任务已经提交"},   {server_task_info_status::assigned, "任务已经分配"},
      {server_task_info_status::completed, "任务已经被完成"}, {server_task_info_status::canceled, "任务已经被取消"},
      {server_task_info_status::failed, "任务已经失败"},      {server_task_info_status::unknown, "未知状态"},
  };
  auto l_status = in_json.get<server_task_info_status>();
  auto l_status_it =
      std::find_if(std::begin(m), std::end(m), [&](auto&& in_pair) { return in_pair.first == l_status; });
  std::string l_status_str = l_status_it != std::end(m) ? l_status_it->second : "未知状态"s;
  return l_status_str;
}

render_monitor::~render_monitor() { p_i->signal_.emit(boost::asio::cancellation_type::all);
}
} // namespace gui
} // namespace doodle