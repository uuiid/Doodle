//
// Created by td_main on 2023/8/22.
//
#pragma once
#include <doodle_app/gui/base/base_window.h>
#include <doodle_app/gui/base/ref_base.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <cstdint>
namespace doodle {
namespace gui {

class render_monitor {
 private:
  struct computer {
    std::uint64_t id_{};
    std::string name_{};
    std::string state_{};
  };
  struct render_task {
    std::uint64_t id_{};
    std::string name_{};
    std::string state_{};
  };

  struct impl {
    bool open_{true};
    gui_cache_name_id component_collapsing_header_id_;
    gui_cache_name_id render_task_collapsing_header_id_;
    gui_cache_name_id component_table_id_;
    gui_cache_name_id render_task_table_id_;
    std::vector<computer> computers_;
    std::vector<render_task> render_tasks_;
  };
  std::unique_ptr<impl> p_i;

 public:
  render_monitor() : p_i(std::make_unique<impl>()){};
  ~render_monitor() = default;

  constexpr static std::string_view name{gui::config::menu_w::render_monitor};

  void init();
  bool render();
  const std::string& title() const;
};

}  // namespace gui
}  // namespace doodle
