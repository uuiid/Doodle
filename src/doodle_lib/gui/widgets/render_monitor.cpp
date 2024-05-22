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
  p_i->logger_timer_ptr_ = std::make_shared<timer_t>(*p_i->strand_ptr_);

  p_i->progress_message_ = "正在查找服务器";
  p_i->logger_ptr_       = g_logger_ctrl().make_log("render_monitor");
  do_find_server_address();
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
          get_logger();
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
          delete_task(l_render_task.id_);
          get_remote_data();
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
          get_logger();
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

void render_monitor::do_find_server_address() {
  p_i->http_client_core_ptr_ = std::make_shared<http::http_client_core>(register_file_type::get_server_address());
  p_i->progress_message_     = "正在查找服务器数据...";
  get_remote_data();
}

void render_monitor::do_wait() {
  if (!open_) return;
  if (app_base::GetPtr()->is_stop()) return;

  p_i->timer_ptr_->expires_after(doodle::chrono::seconds{3});
  p_i->timer_ptr_->async_wait(boost::asio::bind_cancellation_slot(
      app_base::GetPtr()->on_cancel.slot(),
      [this, self = shared_from_this()](boost::system::error_code ec) {
        if (!open_) return;
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
                                                  const boost::beast::http::response<http::basic_json_body>& in_res
                                              ) {
                                                if (in_code) {
                                                  log_error(p_i->logger_ptr_, fmt::format("{}", in_code));
                                                  p_i->progress_message_ = fmt::format("{}", in_code);
                                                  return;
                                                }
                                                p_i->progress_message_.clear();

                                                if (in_res.result() != boost::beast::http::status::ok) {
                                                  p_i->progress_message_ =
                                                      fmt::format("错误 {}", enum_to_num(in_res.result()));
                                                  return;
                                                }
                                                auto l_json = in_res.body();
                                                p_i->computers_.clear();
                                                for (auto&& l_c : l_json) {
                                                  p_i->computers_.emplace_back(
                                                      l_c["name"].get<std::string>(), l_c["ip"].get<std::string>(),
                                                      l_c["status"].get<std::string>()
                                                  );
                                                }
                                              }
                                          )
                      )
  );
  boost::beast::http::request<boost::beast::http::empty_body> l_req_task{boost::beast::http::verb::get, "v1/task", 11};
  l_req_task.keep_alive(true);
  l_req_task.set(boost::beast::http::field::accept, "application/json");

  p_i->http_client_core_ptr_->async_read<boost::beast::http::response<http::basic_json_body>>(
      l_req_task,
      boost::asio::bind_executor(
          g_io_context(), boost::asio::bind_cancellation_slot(
                              app_base::GetPtr()->on_cancel.slot(),
                              [this, self = shared_from_this()](
                                  const boost::system::error_code& in_code,
                                  const boost::beast::http::response<http::basic_json_body>& in_res
                              ) {
                                if (in_code) {
                                  log_error(p_i->logger_ptr_, fmt::format("{}", in_code));
                                  p_i->progress_message_ = fmt::format("{}", in_code);
                                  do_wait();
                                  return;
                                }
                                p_i->progress_message_.clear();

                                if (in_res.result() != boost::beast::http::status::ok) {
                                  p_i->progress_message_ = fmt::format("错误 {}", enum_to_num(in_res.result()));
                                  return;
                                }
                                auto l_json = in_res.body();
                                p_i->render_tasks_.clear();
                                for (auto&& l_c : l_json) {
                                  auto& l_task = p_i->render_tasks_.emplace_back(
                                      l_c["id"].get<std::int32_t>(), fmt::format("{}", l_c["id"].get<std::int32_t>()),
                                      l_c["name"].get<std::string>(), conv_state(l_c["status"]),
                                      l_c["source_computer"].get<std::string>(), l_c["submitter"].get<std::string>(),
                                      l_c["submit_time"].get<std::string>(), l_c["run_computer"].get<std::string>(),
                                      l_c["run_computer_ip"].get<std::string>(), conv_time(l_c["run_time"]),
                                      conv_time(l_c["end_time"])
                                  );
                                  l_task.delete_button_id_ = fmt::format("删除##{}", l_task.id_);
                                  l_task.parse_time();
                                  l_task.update_duration();
                                }

                                do_wait();
                              }
                          )
      )
  );
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

void render_monitor::get_logger() {
  if (!p_i->current_select_logger_) return;
  boost::urls::url l_url{};
  l_url.set_path(fmt::format("v1/task/{}/log", *p_i->current_select_logger_));
  l_url.set_encoded_query(fmt::format("level={}", magic_enum::enum_name(p_i->index_)));
  // get logger
  boost::beast::http::request<boost::beast::http::empty_body> l_logger_get{
      boost::beast::http::verb::get, l_url.c_str(), 11
  };
  l_logger_get.keep_alive(true);
  l_logger_get.set(boost::beast::http::field::accept, "application/json");

  p_i->http_client_core_ptr_->async_read<boost::beast::http::response<boost::beast::http::string_body>>(
      l_logger_get,
      boost::asio::bind_executor(
          g_io_context(), boost::asio::bind_cancellation_slot(
                              app_base::GetPtr()->on_cancel.slot(),
                              [this, self = shared_from_this()](
                                  const boost::system::error_code& in_code,
                                  const boost::beast::http::response<boost::beast::http::string_body>& in_res
                              ) {
                                if (in_code) {
                                  log_error(p_i->logger_ptr_, fmt::format("{}", in_code));
                                  p_i->progress_message_ = fmt::format("{}", in_code);
                                  p_i->logger_data.clear();
                                  return;
                                }
                                p_i->progress_message_.clear();

                                if (in_res.result() != boost::beast::http::status::ok) {
                                  p_i->logger_ptr_->error("错误{}", enum_to_num(in_res.result()));
                                  p_i->logger_data.clear();
                                  return;
                                }
                                p_i->logger_data = std::move(in_res.body());
                              }
                          )
      )
  );
}
void render_monitor::do_wait_logger() {
  if (!open_) return;
  if (app_base::GetPtr()->is_stop()) return;
  p_i->logger_timer_ptr_->expires_after(doodle::chrono::seconds{3});
  p_i->logger_timer_ptr_->async_wait(boost::asio::bind_cancellation_slot(
      app_base::GetPtr()->on_cancel.slot(),
      [this, self = shared_from_this()](boost::system::error_code ec) {
        if (!open_) return;
        if (ec) {
          p_i->logger_ptr_->error("timer_ptr_ error: {}", ec);
          return;
        }
        get_logger();
      }
  ));
}
void render_monitor::delete_task(const std::int32_t in_id) {
  boost::urls::url l_url{};
  l_url.set_path(fmt::format("v1/task/{}", in_id));
  // get logger
  boost::beast::http::request<boost::beast::http::empty_body> l_logger_get{
      boost::beast::http::verb::delete_, l_url.c_str(), 11
  };
  l_logger_get.keep_alive(true);

  p_i->http_client_core_ptr_->async_read<boost::beast::http::response<boost::beast::http::string_body>>(
      l_logger_get,
      boost::asio::bind_executor(
          g_io_context(), boost::asio::bind_cancellation_slot(
                              app_base::GetPtr()->on_cancel.slot(),
                              [this, self = shared_from_this()](
                                  const boost::system::error_code& in_code,
                                  const boost::beast::http::response<boost::beast::http::string_body>& in_res
                              ) {
                                if (in_code) {
                                  log_error(p_i->logger_ptr_, fmt::format("{}", in_code));
                                  p_i->progress_message_ = fmt::format("{}", in_code);
                                  return;
                                }
                                p_i->progress_message_.clear();

                                if (in_res.result() != boost::beast::http::status::ok) {
                                  p_i->logger_ptr_->error("错误{}", enum_to_num(in_res.result()));
                                  return;
                                }
                              }
                          )
      )
  );
}

render_monitor::~render_monitor() = default;
}  // namespace gui
}  // namespace doodle