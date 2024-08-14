//
// Created by TD on 2022/9/21.
//

#pragma once

#include <doodle_app/gui/base/base_window.h>
#include <doodle_app/gui/base/ref_base.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_client/render_client.h>

#include <vector>

namespace doodle::gui {

class DOODLELIB_API maya_tool {
  FSys::path p_cloth_path{};

  struct path_info_t {
    FSys::path path_;
    episodes episode_;
    shot shot_;
    project project_;
  };
  std::vector<path_info_t> path_info_{};

  bool p_use_all_ref{};
  bool p_upload_files{};
  std::string title_name_{};
  bool open{true};
  class impl;
  std::unique_ptr<impl> ptr_attr;
  std::shared_ptr<render_client::client> p_render_client{};

  void set_path(const std::vector<FSys::path>& in_path);

  entt::handle analysis_path(const path_info_t& in_path);
  std::set<FSys::path> list_sim_file(const project& in_project);

  void post_http_task(const std::vector<nlohmann::json>& in_task);
  void open_mir();

 public:
  maya_tool();
  virtual ~maya_tool();
  constexpr static std::string_view name{gui::config::menu_w::comm_maya_tool};

  const std::string& title() const;
  bool render();
};

}  // namespace doodle::gui
