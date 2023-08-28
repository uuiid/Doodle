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
  struct computer {
    std::int64_t id_{};
    std::string name_{};
    std::string state_{};
  };
  struct render_task {
    std::int64_t id_{};
    std::string name_{};
    std::string state_{};
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
    std::vector<computer> computers_{};
    std::vector<render_task> render_tasks_{};
    std::shared_ptr<client> client_ptr_{};
    strand_ptr_t strand_ptr_{};
    timer_ptr_t timer_ptr_{};
    std::once_flag once_flag_{};
  };
  std::unique_ptr<impl> p_i;

  void do_wait();

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
