//
// Created by td_main on 2023/4/27.
//

#include "export_file_fbx.h"

#include "doodle_core/core/core_help_impl.h"
#include <doodle_core/doodle_core_fwd.h>

#include "maya_plug_fwd.h"
#include <maya_plug/data/maya_camera.h>
#include <maya_plug/data/reference_file.h>

#include <memory>

namespace doodle::maya_plug {

void export_file_fbx::export_anim(const entt::handle_view<reference_file, generate_file_path_ptr>& in_handle_view) {}
void export_file_fbx::export_sim(const entt::handle_view<reference_file, generate_file_path_ptr>& in_handle_view) {}
void export_file_fbx::export_cam(const entt::handle_view<generate_file_path_ptr>& in_handle_view) {
  auto& l_arg = in_handle_view.get<generate_file_path_ptr>();
  auto& l_cam = g_reg()->ctx().get<maya_camera>();
  l_cam.unlock_attr();
  l_cam.back_camera(l_arg->begin_end_time.first, l_arg->begin_end_time.second);
  DOODLE_LOG_INFO("开始检查相机是否在世界下方 {}", k_cam.get_transform_name());
  if (l_cam.camera_parent_is_word()) {
    l_cam.fix_group_camera(l_arg->begin_end_time.first, l_arg->begin_end_time.second);
  }
  g_reg()->ctx().get<maya_camera>().export_file(
      l_arg->begin_end_time.first, l_arg->begin_end_time.second,
      *std::dynamic_pointer_cast<reference_file_ns::generate_fbx_file_path>(l_arg)
  );
}

}  // namespace doodle::maya_plug