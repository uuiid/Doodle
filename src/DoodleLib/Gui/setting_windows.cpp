//
// Created by TD on 2021/9/15.
//

#include "setting_windows.h"

#include <DoodleLib/core/CoreSet.h>
#include <DoodleLib/libWarp/imgui_warp.h>

#include <magic_enum.hpp>

namespace doodle {

setting_windows::setting_windows()
    : p_dep_list(magic_enum::enum_names<Department>()),
      p_cur_dep_index(magic_enum::enum_integer(CoreSet::getSet().getDepartmentEnum())),
      p_user(new_object<std::string>(CoreSet::getSet().getUser())),
      p_cache(new_object<std::string>(CoreSet::getSet().getCacheRoot().generic_string())),
      p_doc(new_object<std::string>(CoreSet::getSet().getDoc().generic_string())),
      p_maya_path(new_object<std::string>(CoreSet::getSet().MayaPath().generic_string())),
      p_ue_path(new_object<std::string>(CoreSet::getSet().gettUe4Setting().Path().generic_string())),
      p_ue_version(new_object<std::string>(CoreSet::getSet().gettUe4Setting().Version())),
      p_batch_max(new_object<std::int32_t>(std::thread::hardware_concurrency())) {
}
void setting_windows::frame_render(const bool_ptr& is_show) {
  dear::Begin{"设置窗口", is_show.get()} && [this]() {
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

    if (imgui::Button("save"))
      save();
  };
}
void setting_windows::save() {
  auto& set = CoreSet::getSet();
  set.setDepartment(magic_enum::enum_cast<Department>(p_cur_dep_index).value());
  set.setUser(*p_user);
  set.setMayaPath(*p_maya_path);
  set.gettUe4Setting().setPath(*p_ue_path);
  set.gettUe4Setting().setVersion(*p_ue_version);
  set.writeDoodleLocalSet();
}
}  // namespace doodle
