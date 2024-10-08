//
// Created by td_main on 2023/8/22.
//
#pragma once
#include <doodle_core/core/http_client_core.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_app/gui/base/base_window.h>
#include <doodle_app/gui/base/ref_base.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_client/render_client.h>

#include <boost/asio.hpp>

#include <cstdint>
namespace doodle {
namespace gui {

class render_monitor {
 private:
  struct computer_gui {
    std::string name_{};
    std::string ip_{};
    std::string status_{};
  };
  struct task_t_gui {
    uuid id_{};
    std::string id_str_{};
    std::string name_{};
    std::string status_{};

    std::string source_computer_{};
    std::string submitter_{};
    std::string submit_time_{};

    std::string run_computer_{};
    std::string run_computer_ip_{};
    std::string run_time_{};
    std::string end_time_{};

    chrono::sys_time_pos::clock::time_point start_time_point_{};
    chrono::sys_time_pos::clock::time_point end_time_point_{};
    // 计算持续时间
    std::string duration_{};
    std::string delete_button_id_{};
    // 从字符串解析时间
    void parse_time();

    // 计算持续时间
    void update_duration();
  };
  using timer_t = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>::as_default_on_t<boost::asio::system_timer>;
  using timer_ptr_t       = std::shared_ptr<timer_t>;
  using strand_t          = boost::asio::strand<boost::asio::io_context::executor_type>;
  using strand_ptr_t      = std::shared_ptr<strand_t>;
  using client_t          = render_client::client;
  using http_client_ptr_t = std::shared_ptr<client_t>;

  struct impl {
    gui_cache_name_id component_collapsing_header_id_{"渲染注册"};
    gui_cache_name_id render_task_collapsing_header_id_{"渲染任务"};
    gui_cache_name_id component_table_id_{"注册列表"s};
    gui_cache_name_id render_task_table_id_{"任务列表"s};
    // 进度条
    gui_cache_name_id progress_bar_id_{"进度条"s};
    // 进度信息
    std::string progress_message_{};
    // 计算机列表
    std::vector<computer_gui> computers_{};
    // 任务列表
    std::vector<task_t_gui> render_tasks_{};
    // logger
    gui_cache_name_id logger_collapsing_header_id_{"日志"};
    gui_cache_name_id logger_child_id_{"日志"s};
    level::level_enum index_{level::warn};
    gui_cache<std::string> logger_level_{"日志等级", magic_enum::enum_name(level::err)};
    std::string logger_data{};
    std::optional<uuid> current_select_logger_{};

    strand_ptr_t strand_ptr_{};
    timer_ptr_t timer_ptr_{};
    std::once_flag once_flag_{};

    // 分页索引
    gui_cache_name_id page_index_id_{"分页索引"};
    int32_t page_index_{0};
    // 最大页数
    int32_t max_page_num_{0};

    // logger
    logger_ptr logger_ptr_{};

    // 客户端
    http_client_ptr_t http_client_ptr_{};
    boost::asio::cancellation_signal signal_{};
  };
  std::shared_ptr<impl> p_i;

  // 刷新
  boost::asio::awaitable<void> async_refresh();
  boost::asio::awaitable<void> async_refresh_task();
  boost::asio::awaitable<void> async_delete_task(uuid in_id);
  boost::asio::awaitable<void> async_refresh_logger();

  static std::string conv_time(const nlohmann::json& in_json);
  static std::string conv_state(const nlohmann::json& in_json);

 public:
  render_monitor() : p_i(std::make_shared<impl>()){};
  ~render_monitor();

  constexpr static std::string_view name{gui::config::menu_w::render_monitor};
  bool open_{true};

  void init();
  bool render();
  const std::string& title() const;
};

}  // namespace gui
}  // namespace doodle
