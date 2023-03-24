
//
// Created by TD on 2021/9/15.
//

#include "setting_windows.h"

#include "doodle_core/core/core_set.h"
#include "doodle_core/core/doodle_lib.h"
#include "doodle_core/core/init_register.h"
#include "doodle_core/doodle_core.h"
#include "doodle_core/metadata/metadata.h"
#include "doodle_core/metadata/user.h"

#include "doodle_app/gui/base/ref_base.h"
#include "doodle_app/gui/show_message.h"
#include "doodle_app/lib_warp/imgui_warp.h"

#include <boost/numeric/conversion/cast.hpp>

#include <magic_enum.hpp>
namespace doodle::gui {

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
        p_batch_max("最大任务数"s, std::int32_t{core_set::get_set().p_max_thread}),
        p_timeout("任务超时时间"s, boost::numeric_cast<std::int32_t>(core_set::get_set().timeout)) {}
  gui::gui_cache<std::string> p_user;
  gui::gui_cache<std::string> p_org_name;
  gui::gui_cache<std::string> p_cache;
  gui::gui_cache<std::string> p_doc;
  gui::gui_cache<std::string> p_maya_path;
  gui::gui_cache<std::string> p_ue_path;
  gui::gui_cache<std::string> p_ue_version;
  gui::gui_cache<std::int32_t> p_batch_max;
  gui::gui_cache<std::int32_t> p_timeout;
  gui::gui_cache<bool> p_maya_replace_save_dialog{"替换maya默认对话框"s, core_set::get_set().maya_replace_save_dialog};
  gui::gui_cache<bool> p_maya_force_resolve_link{"强制maya解析链接"s, core_set::get_set().maya_force_resolve_link};
  std::string user_uuid;
  gui::gui_cache_name_id new_user_id{"生成新id"s};
  std::string title_name_;
  bool open;
};

setting_windows::setting_windows() : p_i(std::make_unique<impl>()) { p_i->title_name_ = std::string{name}; }

void setting_windows::save() {
  auto& set                    = core_set::get_set();

  set.organization_name        = p_i->p_org_name.data;
  set.p_mayaPath               = p_i->p_maya_path.data;
  set.p_max_thread             = p_i->p_batch_max.data;
  set.ue4_path                 = p_i->p_ue_path.data;
  set.ue4_version              = p_i->p_ue_version.data;
  set.timeout                  = p_i->p_timeout.data;
  set.maya_replace_save_dialog = p_i->p_maya_replace_save_dialog.data;
  set.maya_force_resolve_link  = p_i->p_maya_force_resolve_link.data;

  auto&& l_u                   = g_reg()->ctx().at<user::current_user>();
  l_u.user_name_attr(p_i->p_user());
  g_reg()->ctx().at<core_sig>().save();
  core_set_init{}.write_file();
}
setting_windows::~setting_windows() = default;

void setting_windows::init() {
  auto l_user                          = g_reg()->ctx().at<user::current_user>().get_handle();
  p_i->p_user.data                     = l_user.get<user>().get_name();
  p_i->user_uuid                       = fmt::format("用户id: {}", l_user.get<database>().uuid());

  p_i->p_org_name.data                 = core_set::get_set().organization_name;
  p_i->p_cache.data                    = core_set::get_set().get_cache_root().generic_string();
  p_i->p_doc.data                      = core_set::get_set().get_doc().generic_string();
  p_i->p_maya_path.data                = core_set::get_set().maya_path().generic_string();
  p_i->p_ue_path.data                  = core_set::get_set().ue4_path.generic_string();
  p_i->p_ue_version.data               = core_set::get_set().ue4_version;
  p_i->p_batch_max.data                = core_set::get_set().p_max_thread;
  p_i->p_timeout.data                  = core_set::get_set().timeout;
  p_i->p_maya_replace_save_dialog.data = core_set::get_set().maya_replace_save_dialog;
  p_i->p_maya_force_resolve_link.data  = core_set::get_set().maya_force_resolve_link;
}

bool setting_windows::render() {
  const dear::Begin l_wind{p_i->title_name_.data(), &p_i->open};
  if (!l_wind) return p_i->open;

  ImGui::InputText(*p_i->p_org_name.gui_name, &p_i->p_org_name.data);
  imgui::InputText(*p_i->p_user.gui_name, &(p_i->p_user.data));
  dear::Text(p_i->user_uuid);
  ImGui::SameLine();
  if (ImGui::Button(*p_i->new_user_id)) {
    g_reg()->ctx().at<user::current_user>() = {};
    auto& l_h                               = g_reg()->ctx().at<user::current_user>();
    l_h.create_user();
    p_i->user_uuid = fmt::format("用户id: {}", l_h.uuid);
  }

  imgui::InputText(*p_i->p_cache.gui_name, &(p_i->p_cache.data), ImGuiInputTextFlags_ReadOnly);
  imgui::InputText(*p_i->p_doc.gui_name, &(p_i->p_doc.data), ImGuiInputTextFlags_ReadOnly);
  imgui::InputText(*p_i->p_maya_path.gui_name, &(p_i->p_maya_path.data));
  imgui::InputText(*p_i->p_ue_path.gui_name, &(p_i->p_ue_path.data));
  imgui::InputText(*p_i->p_ue_version.gui_name, &(p_i->p_ue_version.data));
  imgui::InputInt(*p_i->p_batch_max.gui_name, &(p_i->p_batch_max.data));
  dear::HelpMarker{"更改任务池时,减小不会结束现在的任务, 真假时会立即加入等待的项目"s};
  imgui::InputInt(*p_i->p_timeout.gui_name, &(p_i->p_timeout.data));
  imgui::Checkbox(*p_i->p_maya_replace_save_dialog.gui_name, &(p_i->p_maya_replace_save_dialog.data));
  imgui::Checkbox(*p_i->p_maya_force_resolve_link.gui_name, &(p_i->p_maya_force_resolve_link.data));
  dear::HelpMarker{"强制maya解析硬链接, 这个是在插件中使用的选项"s};

  if (imgui::Button("save")) save();

  return p_i->open;
}

const std::string& gui::setting_windows::title() const { return p_i->title_name_; }

}  // namespace doodle::gui
