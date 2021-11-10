//
// Created by TD on 2021/11/04.
//

#include "command_down_file.h"

#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata_cpp.h>
#include <doodle_lib/rpc/rpc_file_system_client.h>
namespace doodle {
comm_file_down::comm_file_down()
    : p_root(),
      p_str(new_object<string>()) {
  p_show_str = make_imgui_name(this, "下载路径", "下载");
}

bool comm_file_down::render() {
  if (p_root) {
    imgui::InputText(p_show_str["下载路径"].c_str(), p_str.get());
    if (imgui::Button(p_show_str["下载"].c_str())) {
      auto& k_f   = p_root.get<assets_path_vector>();
      auto k_down = doodle_lib::Get().get_rpc_file_system_client()->download(k_f.make_down_path(*p_str));
      (*k_down)();
    }
  }
  return false;
}
bool comm_file_down::set_data(const entt::handle& in_any) {
  if (in_any.all_of<assets_path_vector>()) {
    p_root = in_any;
    *p_str = p_root.get<assets_path_vector>().get_cache_path().generic_string();
  } else {
    p_root = entt::handle{};
    return false;
  }
  return true;
}
}  // namespace doodle
