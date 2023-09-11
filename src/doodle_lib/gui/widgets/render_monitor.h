//
// Created by td_main on 2023/8/22.
//
#pragma once
#include <doodle_app/gui/base/base_window.h>
#include <doodle_app/gui/base/ref_base.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/render_farm/client.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <cstdint>
namespace doodle {
namespace gui {

class render_monitor : public std::enable_shared_from_this<render_monitor> {
 private:
  struct computer_gui {
    explicit computer_gui(client::computer in_computer)
        : computer_(std::move(in_computer)), id_{fmt::to_string(computer_.id_)} {}
    client::computer computer_{};
    std::string id_{};
  };
  struct task_t_gui {
    explicit task_t_gui(client::task_t in_task) : task_(std::move(in_task)), id_{fmt::to_string(task_.id_)} {}
    client::task_t task_{};
    std::string id_{};
  };
  using timer_t      = boost::asio::system_timer;
  using timer_ptr_t  = std::shared_ptr<timer_t>;
  using strand_t     = boost::asio::strand<boost::asio::io_context::executor_type>;
  using strand_ptr_t = std::shared_ptr<strand_t>;

  struct impl {
    gui_cache_name_id component_collapsing_header_id_{"渲染注册"};
    gui_cache_name_id render_task_collapsing_header_id_{"渲染任务"};
    gui_cache_name_id component_table_id_{"注册列表"s};
    gui_cache_name_id render_task_table_id_{"任务列表"s};
    // 进度条
    gui_cache_name_id progress_bar_id_{"进度条"s};
    // 进度信息
    std::float_t progress_{};
    std::string progress_message_{};
    std::vector<computer_gui> computers_{};
    std::vector<task_t_gui> render_tasks_{};
    std::shared_ptr<client> client_ptr_{};
    strand_ptr_t strand_ptr_{};
    timer_ptr_t timer_ptr_{};
    udp_client_ptr udp_client_ptr_{};
    std::once_flag once_flag_{};

    // logger
    logger_ptr logger_ptr_{};
  };
  std::unique_ptr<impl> p_i;

  void do_wait();
  void do_find_server_address();

  void get_remote_data();

 public:
  render_monitor() : p_i(std::make_unique<impl>()){};
  ~render_monitor();

  constexpr static std::string_view name{gui::config::menu_w::render_monitor};
  bool open_{true};

  void init();
  bool render();
  const std::string& title() const;
};

}  // namespace gui
}  // namespace doodle
