//
// Created by TD on 2021/9/15.
//

#include "setting_windows.h"

#include "doodle_core/core/core_set.h"
#include "doodle_core/core/doodle_lib.h"
#include "doodle_core/doodle_core.h"
#include "doodle_core/logger/logger.h"
#include "doodle_core/metadata/metadata.h"
#include "doodle_core/metadata/user.h"
#include <doodle_core/database_task/sqlite_client.h>

#include "doodle_app/gui/base/ref_base.h"
#include "doodle_app/gui/show_message.h"
#include "doodle_app/lib_warp/imgui_warp.h"

#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>

#include <boost/numeric/conversion/cast.hpp>

#include <cstdint>
#include <fmt/core.h>
#include <imgui.h>
#include <magic_enum.hpp>
#include <string>
#include <winreg/WinReg.hpp>

namespace doodle::gui {
class setting_windows::impl {
public:
  impl()
    : p_user("用户"s, ""s),
      p_org_name("部门"s, ""s),
      p_cache("缓存位置"s, ""s),
      p_doc("文档路径"s, ""s),
      p_maya_path("maya 版本"s, 2019),
      p_ue_path("ue路径"s, ""s),
      p_ue_version("ue版本"s),
      p_batch_max("最大任务数"s, std::int32_t{core_set::get_set().p_max_thread}),
      p_timeout("任务超时时间"s, boost::numeric_cast<std::int32_t>(core_set::get_set().timeout)) {
  }

  gui::gui_cache<std::string> p_user;
  gui::gui_cache<std::string> p_org_name;
  gui::gui_cache<std::string> p_cache;
  gui::gui_cache<std::string> p_doc;
  gui::gui_cache<std::int32_t> p_maya_path;
  gui::gui_cache<std::string> p_ue_path;
  std::string p_ue_version;
  gui::gui_cache<std::int32_t> p_batch_max;
  gui::gui_cache<std::int32_t> p_timeout;
  std::string maya_path;
  gui::gui_cache<bool> p_maya_replace_save_dialog{"替换maya默认对话框"s, core_set::get_set().maya_replace_save_dialog};
  gui::gui_cache<bool> p_maya_force_resolve_link{"强制maya解析链接"s, core_set::get_set().maya_force_resolve_link};
  std::string user_uuid;
  gui::gui_cache_name_id new_user_id{"切换新用户"s};
  std::string title_name_;
  // 欢迎窗口显示
  gui_cache<bool> welcome_show{"不显示初始窗口"s, true};
  bool open{true};
};

setting_windows::setting_windows() : p_i(std::make_unique<impl>()) {
  p_i->title_name_ = std::string{name};

  init();
}

void setting_windows::save() {
  auto& set = core_set::get_set();

  set.organization_name        = p_i->p_org_name.data;
  set.maya_version             = p_i->p_maya_path.data;
  set.p_max_thread             = p_i->p_batch_max.data;
  set.ue4_path                 = p_i->p_ue_path.data;
  set.ue4_version              = p_i->p_ue_version;
  set.timeout                  = p_i->p_timeout.data;
  set.maya_replace_save_dialog = p_i->p_maya_replace_save_dialog.data;
  set.maya_force_resolve_link  = p_i->p_maya_force_resolve_link.data;
  set.next_time_               = p_i->welcome_show;

  auto&& l_u = g_reg()->ctx().get<user::current_user>();
  l_u.user_name_attr(p_i->p_user());
}

setting_windows::~setting_windows() = default;

void setting_windows::init() {
  auto l_user      = g_reg()->ctx().get<user::current_user>().get_handle();
  p_i->p_user.data = l_user.get<user>().get_name();
  p_i->user_uuid   = fmt::format("用户id: {}", l_user.get<database>().get_id());

  p_i->p_org_name.data                 = core_set::get_set().organization_name;
  p_i->p_cache.data                    = core_set::get_set().get_cache_root().generic_string();
  p_i->p_doc.data                      = core_set::get_set().get_doc().generic_string();
  p_i->p_maya_path.data                = core_set::get_set().maya_version;
  p_i->p_ue_path.data                  = core_set::get_set().ue4_path.generic_string();
  p_i->p_ue_version                    = core_set::get_set().ue4_version;
  p_i->p_batch_max.data                = core_set::get_set().p_max_thread;
  p_i->p_timeout.data                  = core_set::get_set().timeout;
  p_i->p_maya_replace_save_dialog.data = core_set::get_set().maya_replace_save_dialog;
  p_i->p_maya_force_resolve_link.data  = core_set::get_set().maya_force_resolve_link;
  p_i->welcome_show                    = core_set::get_set().next_time_;
}

bool setting_windows::render() {
  ImGui::InputText(*p_i->p_org_name.gui_name, &p_i->p_org_name.data);
  imgui::InputText(*p_i->p_user.gui_name, &(p_i->p_user.data));
  dear::Text(p_i->user_uuid);
  ImGui::SameLine();
  if (ImGui::Button(*p_i->new_user_id)) {
    g_reg()->ctx().get<user::current_user>() = {};
    auto& l_h                                = g_reg()->ctx().get<user::current_user>();
    l_h.create_user();
    p_i->user_uuid = fmt::format("用户id: {}", l_h.user_handle.get<database>().get_id());
  }

  imgui::InputText(*p_i->p_cache.gui_name, &(p_i->p_cache.data), ImGuiInputTextFlags_ReadOnly);
  imgui::InputText(*p_i->p_doc.gui_name, &(p_i->p_doc.data), ImGuiInputTextFlags_ReadOnly);
  if (imgui::InputInt(*p_i->p_maya_path.gui_name, &(p_i->p_maya_path.data))) {
    core_set::get_set().maya_version = p_i->p_maya_path.data;
  };
  ImGui::SameLine();
  if (ImGui::Button("测试寻找")) {
    boost::system::error_code l_error_code{};
    try {
      p_i->maya_path = maya_exe_ns::find_maya_path().generic_string();
    } catch (...) {
      p_i->maya_path = fmt::format("没有找到maya文件的运行程序({})", boost::current_exception_diagnostic_information());
    }
  }
  constexpr static auto g_text{"拖拽ue exe文件到此处"};
  if (!p_i->maya_path.empty()) dear::Text(p_i->maya_path);

  if (imgui::InputText(*p_i->p_ue_path.gui_name, &(p_i->p_ue_path.data))) {
    get_ue_version();
  };
  if (auto l_drag          = dear::DragDropTarget{}) {
    if (const auto* l_data = ImGui::AcceptDragDropPayload(doodle_config::drop_imgui_id.data());
      l_data && l_data->IsDelivery()) {
      auto* l_list = static_cast<std::vector<FSys::path>*>(l_data->Data);
      if (!l_list->empty()) {
        p_i->p_ue_path.data = l_list->front().parent_path().parent_path().parent_path().parent_path().generic_string();
        get_ue_version();
      }
    }
  }
  if (auto l_tip = dear::ItemTooltip{}) {
    ImGui::Text(g_text);
  }
  ImGui::Text("ue 版本: ");
  ImGui::SameLine();
  dear::Text(p_i->p_ue_version);
  imgui::InputInt(*p_i->p_batch_max.gui_name, &(p_i->p_batch_max.data));
  dear::HelpMarker{"更改任务池时,减小不会结束现在的任务, 真假时会立即加入等待的项目"s};
  imgui::InputInt(*p_i->p_timeout.gui_name, &(p_i->p_timeout.data));
  imgui::Checkbox(*p_i->p_maya_replace_save_dialog.gui_name, &(p_i->p_maya_replace_save_dialog.data));
  imgui::Checkbox(*p_i->p_maya_force_resolve_link.gui_name, &(p_i->p_maya_force_resolve_link.data));
  dear::HelpMarker{"强制maya解析硬链接, 这个是在插件中使用的选项"s};
  imgui::Checkbox(*p_i->welcome_show.gui_name, &(p_i->welcome_show.data));

  if (imgui::Button("save")) save();

  return p_i->open;
}

const std::string& gui::setting_windows::title() const { return p_i->title_name_; }

void setting_windows::get_ue_version() {
  if (p_i->p_ue_path.data.empty()) return;
  auto l_path = FSys::path{p_i->p_ue_path.data} / "Engine/Binaries/Win64/UnrealEditor.exe";
  if (!FSys::exists(l_path)) {
    p_i->p_ue_version = "没有找到 UnrealEditor.exe";
    return;
  }
  try {
    p_i->p_ue_version = ue_exe_ns::get_file_version(l_path);
  } catch (...) {
    p_i->p_ue_version = boost::current_exception_diagnostic_information();
  }
}
} // namespace doodle::gui