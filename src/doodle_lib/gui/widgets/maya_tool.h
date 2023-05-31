//
// Created by TD on 2022/9/21.
//

#pragma once

#include <doodle_core/core/init_register.h>

#include <doodle_app/gui/base/base_window.h>
#include <doodle_app/gui/base/ref_base.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::gui {

class DOODLELIB_API maya_tool {
  FSys::path p_cloth_path{};
  std::string p_text{};
  std::vector<FSys::path> p_sim_path{};

  bool p_use_all_ref{};
  bool p_upload_files{};
  std::string title_name_{};
  bool open{true};

  class impl;
  std::unique_ptr<impl> ptr_attr;

 public:
  maya_tool();
  virtual ~maya_tool();
  constexpr static std::string_view name{gui::config::menu_w::comm_maya_tool};

  void init();
  const std::string& title() const;
  bool render();
};

}  // namespace doodle::gui
