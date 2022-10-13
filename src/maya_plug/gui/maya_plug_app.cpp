//
// Created by TD on 2021/10/14.
//

#include "maya_plug_app.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/thread_pool/process_pool.h>

#include <doodle_app/app/this_rpc_exe.h>
#include <doodle_app/gui/main_menu_bar.h>
#include <doodle_app/gui/main_proc_handle.h>
#include <doodle_app/gui/main_status_bar.h>

#include <maya_plug/data/null_facet.h>
#include <maya_plug/gui/maya_layout.h>

#include <maya/MGlobal.h>

namespace doodle::maya_plug {
void maya_facet::load_windows() {
  g_reg()->ctx().at<gui::main_proc_handle>().win_close   = [this]() { this->close_windows(); };
  g_reg()->ctx().at<gui::main_proc_handle>().win_destroy = []() {};
  make_handle().emplace<gui::gui_tick>(std::make_shared<maya_layout>());
  make_handle().emplace<gui::gui_tick>(std::make_shared<maya_menu>());
  make_handle().emplace<gui::gui_tick>(std::make_shared<gui::main_status_bar>());
}
void maya_facet::close_windows() {
  ::ShowWindow(p_hwnd, SW_HIDE);
}
maya_facet::maya_facet() : doodle::facet::gui_facet() {
  g_reg()->ctx().at<image_to_move>() = std::make_shared<detail::maya_create_movie>();
}

namespace detail {

class maya_create_movie::impl {
 public:
  doodle::detail::this_rpc_exe doodle_exe_attr;
};

maya_create_movie::maya_create_movie() : ptr(std::make_unique<impl>()) {}

void maya_create_movie::create_move(const FSys::path& in_out_path, process_message& in_msg, const std::vector<image_attr>& in_vector) {
  ptr->doodle_exe_attr.create_move(
      in_out_path, in_vector, in_msg
  );
}
FSys::path maya_create_movie::create_out_path(const entt::handle& in_handle) {
  boost::ignore_unused(this);

  FSys::path l_out{};
  l_out = in_handle.get<FSys::path>();

  /// \brief 这里我们检查 shot，episode 进行路径的组合
  if (!l_out.has_extension() && in_handle.any_of<episodes, shot>())
    l_out /= fmt::format(
        "{}_{}.mp4",
        in_handle.any_of<episodes>() ? fmt::to_string(in_handle.get<episodes>()) : "eps_none"s,
        in_handle.any_of<shot>() ? fmt::to_string(in_handle.get<shot>()) : "sh_none"s
    );
  else if (!l_out.has_extension()) {
    l_out /= fmt::format(
        "{}.mp4", core_set::get_set().get_uuid()
    );
  } else
    l_out.extension() == ".mp4" ? void() : throw_exception(doodle_error{"扩展名称不是MP4"});

  if (exists(l_out.parent_path()))
    create_directories(l_out.parent_path());
  return l_out;
}
}  // namespace detail
maya_plug_app::maya_plug_app() {
  switch (MGlobal::mayaState()) {
    case MGlobal::MMayaState::kBaseUIMode:
    case MGlobal::MMayaState::kInteractive: {
      run_facet = std::make_shared<null_facet>();
      add_facet(run_facet);
    }
    case MGlobal::MMayaState::kBatch:
    case MGlobal::MMayaState::kLibraryApp:
    default: {
      run_facet = std::make_shared<maya_facet>();
      add_facet(run_facet);
    } break;
  }
}
}  // namespace doodle::maya_plug
