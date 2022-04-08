//
// Created by TD on 2021/9/15.
//

#include "setting_windows.h"

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/lib_warp/imgui_warp.h>

#include <doodle_lib/gui/gui_ref/ref_base.h>

#include <magic_enum.hpp>
namespace doodle {

class setting_windows::impl {
 public:
  impl()
      : p_user("用户"s, ""s),
        p_org_name("部门"s, ""s),
        p_cache("缓存位置"s, ""s),
        p_doc("文档路径"s, ""s),
        p_maya_path("maya路径"s, ""s),
        p_ue_path("ue路径"s, ""s),
        p_ue_version("ue版本"s, ""s),
        p_batch_max("最大任务数"s, core_set::getSet().p_max_thread),
        p_timeout("任务超时时间"s, core_set::getSet().timeout),
        p_max_reg("并行序列化数"s, core_set::getSet().max_install_reg_entt) {}
  gui::gui_cache<std::string> p_user;
  gui::gui_cache<std::string> p_org_name;
  gui::gui_cache<std::string> p_cache;
  gui::gui_cache<std::string> p_doc;
  gui::gui_cache<std::string> p_maya_path;
  gui::gui_cache<std::string> p_ue_path;
  gui::gui_cache<std::string> p_ue_version;
  gui::gui_cache<std::int32_t> p_batch_max;
  gui::gui_cache<std::int32_t> p_timeout;
  gui::gui_cache<std::int32_t> p_max_reg;
};

setting_windows::setting_windows()
    : p_i(std::make_unique<impl>()) {
}

void setting_windows::save() {
  auto& set = core_set::getSet();

  set.set_user(p_i->p_user.data);
  set.organization_name = p_i->p_org_name.data;
  set.p_mayaPath = p_i->p_maya_path.data;
  set.p_max_thread = p_i->p_batch_max.data;
  set.ue4_path = p_i->p_ue_path.data;
  set.ue4_version = p_i->p_ue_version.data;
  set.max_install_reg_entt  = boost::numeric_cast<std::uint16_t>(p_i->p_max_reg.data);
  set.timeout               = p_i->p_timeout.data;
  g_bounded_pool().timiter_ = p_i->p_batch_max.data;
  core_set_init{}.write_file();
}
setting_windows::~setting_windows() = default;

void setting_windows::init() {
  p_i->p_user.data       = core_set::getSet().get_user();
  p_i->p_org_name.data   = core_set::getSet().organization_name;
  p_i->p_cache.data      = core_set::getSet().get_cache_root().generic_string();
  p_i->p_doc.data        = core_set::getSet().get_doc().generic_string();
  p_i->p_maya_path.data  = core_set::getSet().maya_path().generic_string();
  p_i->p_ue_path.data    = core_set::getSet().ue4_path.generic_string();
  p_i->p_ue_version.data = core_set::getSet().ue4_version;
  p_i->p_batch_max.data  = core_set::getSet().p_max_thread;
  p_i->p_timeout.data    = core_set::getSet().timeout;
  p_i->p_max_reg.data    = core_set::getSet().max_install_reg_entt;
  g_reg()->set<setting_windows&>(*this);
}
void setting_windows::succeeded() {
  g_reg()->unset<setting_windows&>();
  save();
}
void setting_windows::failed() {
  g_reg()->unset<setting_windows&>();
  save();
}
void setting_windows::aborted() {
  g_reg()->unset<setting_windows&>();
  save();
}
void setting_windows::update(
    const chrono::duration<chrono::system_clock::rep,
                           chrono::system_clock::period>&,
    void* data) {
  ImGui::InputText(*p_i->p_org_name.gui_name, &p_i->p_org_name.data);
  imgui::InputText(*p_i->p_user.gui_name, &(p_i->p_user.data));
  imgui::InputText(*p_i->p_cache.gui_name, &(p_i->p_cache.data), ImGuiInputTextFlags_ReadOnly);
  imgui::InputText(*p_i->p_doc.gui_name, &(p_i->p_doc.data), ImGuiInputTextFlags_ReadOnly);
  imgui::InputText(*p_i->p_maya_path.gui_name, &(p_i->p_maya_path.data));
  imgui::InputText(*p_i->p_ue_path.gui_name, &(p_i->p_ue_path.data));
  imgui::InputText(*p_i->p_ue_version.gui_name, &(p_i->p_ue_version.data));
  imgui::InputInt(*p_i->p_batch_max.gui_name, &(p_i->p_batch_max.data));
  dear::HelpMarker{"更改任务池时,减小不会结束现在的任务, 真假时会立即加入等待的项目"s};
  imgui::InputInt(*p_i->p_timeout.gui_name, &(p_i->p_timeout.data));
  imgui::InputInt(*p_i->p_max_reg.gui_name, &p_i->p_max_reg.data);
  dear::HelpMarker{"这个选项影响项目的加载速度"s};

  if (imgui::Button("save"))
    save();
}
}  // namespace doodle
