//
// Created by TD on 2021/9/15.
//

#include "setting_windows.h"

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/lib_warp/imgui_warp.h>

#include <magic_enum.hpp>
namespace doodle {

setting_windows::setting_windows()
    : p_dep_list(magic_enum::enum_names<department>()),
      p_cur_dep_index(magic_enum::enum_integer(core_set::getSet().get_department_enum())),
      p_user(new_object<std::string>(core_set::getSet().get_user())),
      p_cache(new_object<std::string>(core_set::getSet().get_cache_root().generic_string())),
      p_doc(new_object<std::string>(core_set::getSet().get_doc().generic_string())),
      p_maya_path(new_object<std::string>(core_set::getSet().maya_path().generic_string())),
      p_ue_path(new_object<std::string>(core_set::getSet().get_ue4_setting().get_path().generic_string())),
      p_ue_version(new_object<std::string>(core_set::getSet().get_ue4_setting().get_version())),
      p_batch_max(new_object<std::int32_t>(core_set::getSet().p_max_thread)),
      p_timeout(new_object<std::int32_t>(core_set::getSet().timeout)) {
  p_class_name = "设置";
}
void setting_windows::frame_render() {
  dear::Combo{"部门", p_dep_list[p_cur_dep_index].data()} && [this]() {
    for (int k_i = 0; k_i < p_dep_list.size(); ++k_i) {
      const bool is_select = p_cur_dep_index == k_i;
      if (dear::Selectable(p_dep_list[k_i].data(), is_select))
        p_cur_dep_index = k_i;

      if (is_select)
        imgui::SetItemDefaultFocus();
    }
  };
  imgui::InputText("用户 ", p_user.get());
  imgui::InputText("缓存", p_cache.get(), ImGuiInputTextFlags_ReadOnly);
  imgui::InputText("文档", p_doc.get(), ImGuiInputTextFlags_ReadOnly);
  imgui::InputText("maya路径", p_maya_path.get());
  imgui::InputText("UE路径", p_ue_path.get());
  imgui::InputText("UE版本", p_ue_version.get());
  imgui::InputInt("batch 操作线程数", p_batch_max.get());
  imgui::SameLine();
  dear::HelpMarker{"更改线程池大小需要一定时间,以及风险"};
  imgui::InputInt("超时结束", p_timeout.get());

  if (imgui::Button("save"))
    save();
}
void setting_windows::save() {
  auto& set = core_set::getSet();
  set.set_department(magic_enum::enum_cast<department>(p_cur_dep_index).value());
  set.set_user(*p_user);
  set.set_maya_path(*p_maya_path);
  set.set_max_tread(*p_batch_max);
  set.get_ue4_setting().set_path(*p_ue_path);
  set.get_ue4_setting().set_version(*p_ue_version);
  set.timeout = *p_timeout;
  g_bounded_pool().set_bounded(*p_batch_max);
  core_set_init{}.write_file();

}
}  // namespace doodle
