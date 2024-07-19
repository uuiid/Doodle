//
// Created by TD on 2022/9/21.
//

#include "maya_tool.h"

#include "doodle_core/configure/static_value.h"
#include <doodle_core/core/app_base.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/redirection_path_info.h>
#include <doodle_core/metadata/season.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/platform/win/register_file_type.h>

#include "doodle_app/lib_warp/imgui_warp.h"
#include <doodle_app/gui/base/ref_base.h>

#include <doodle_lib/core/auto_light_render_video.h>
#include <doodle_lib/core/down_auto_light_anim_file.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/up_auto_light_file.h>
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/gui/widgets/render_monitor.h>

#include "boost/signals2/connection.hpp"

#include "gui/widgets/maya_tool.h"
#include <filesystem>
#include <imgui.h>
#include <utility>
#include <vector>
#include <wil/resource.h>
#include <wil/result.h>
#ifdef _WIN32
#include <boost/process/windows.hpp>
#elif defined __linux__
#include <boost/process/posix.hpp>
#endif
namespace doodle::gui {
namespace {
nlohmann::json create_cgru_json() {
  auto l_json                  = nlohmann::json{};
  auto& l_job_obj              = l_json["job"];
  l_job_obj["name"]            = "test job";
  l_job_obj["user_name"]       = "doodle";
  l_job_obj["host_name"]       = "localhost";
  l_job_obj["priority"]        = 99;
  auto& l_blocks               = l_job_obj["blocks"];
  auto& l_block                = l_blocks[0];
  l_block["flags"]             = 0;
  l_block["name"]              = "block of tasks";
  l_block["service"]           = "generic";
  l_block["capacity"]          = 1000;
  l_block["working_directory"] = "D:/doodle_exe/bin";
  l_block["parser"]            = "generic";
  auto& l_tasks                = l_block["tasks"];
  auto& l_task                 = l_tasks[0];
  l_task["name"]               = "auto light";
  l_task["command"]            = "";
  return l_json;
}

std::string get_user_name() {
  DWORD l_size = 0;
  auto l_err   = ::GetUserNameW(nullptr, &l_size);
  if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    LOG_IF_WIN32_ERROR(::GetLastError());
    return {"doodle"};
  }
  std::unique_ptr<wchar_t[]> l_user_name = std::make_unique<wchar_t[]>(l_size);
  l_err                                  = ::GetUserNameW(l_user_name.get(), &l_size);

  if (FALSE == l_err) {
    LOG_IF_WIN32_ERROR(::GetLastError());
    return {"doodle"};
  }
  l_size = l_size - 1;
  return boost::locale::conv::utf_to_utf<char>(l_user_name.get(), l_user_name.get() + l_size);
}
} // namespace

namespace maya_tool_ns {
enum class maya_type { ma, mb };

class maya_file_type_gui : public gui_cache<maya_type> {
public:
  maya_file_type_gui() : gui_cache<maya_type>("转换文件格式", maya_type::mb) {
  }

  std::string show_id_attr{".mb"};
};

class maya_reference_info : boost::equality_comparable<maya_reference_info> {
public:
  maya_reference_info() = default;

  virtual ~maya_reference_info() {
    if (save_handle) save_handle.destroy();
  }

  gui_cache<std::string> f_file_path_attr{"原始文件路径"s, ""s};
  gui_cache<std::string> to_file_path_attr{"替换文件路径"s, ""s};

  gui_cache_name_id de_button_attr{"删除"};

  entt::handle save_handle{*g_reg(), g_reg()->create()};

  bool operator==(const maya_reference_info& in_r) const {
    return std::tie(f_file_path_attr, to_file_path_attr) == std::tie(in_r.f_file_path_attr, in_r.to_file_path_attr);
  }
};

class ref_attr_gui {
public:
  ref_attr_gui()  = default;
  ~ref_attr_gui() = default;
  gui_cache<std::vector<maya_tool_ns::maya_reference_info>> ref_attr{
      "替换文件设置"s, std::vector<maya_tool_ns::maya_reference_info>{}
  };
  gui_cache_name_id de_button_attr{"添加"};
};
} // namespace maya_tool_ns

class maya_tool::impl {
public:
  gui_cache_name_id convert_maya_id_attr{"转换文件设置"};

  maya_tool_ns::maya_file_type_gui save_maya_type_attr{};
  maya_tool_ns::ref_attr_gui ref_attr{};
  boost::signals2::scoped_connection scoped_connection_1{}, scoped_connection_2{};

  gui_cache<bool> replace_ref_file_{"替换引用"s, true};
  gui_cache<bool> sim_file_{"解算文件"s, true};
  gui_cache<bool> export_abc_type_{"导出abc"s, true};
  gui_cache<bool> create_play_blast_{"创建排屏"s, true};
};

maya_tool::maya_tool() : ptr_attr(std::make_unique<impl>()) {
  if (!g_reg()->ctx().contains<maya_exe_ptr>()) g_reg()->ctx().emplace<maya_exe_ptr>() = std::make_shared<maya_exe>();
  title_name_ = std::string{name};
  init();
}

void maya_tool::set_path(const std::vector<FSys::path>& in_path) {
  auto l_tmp = in_path |
               ranges::views::transform([](const FSys::path& in_handle) -> path_info_t { return {in_handle}; }) |
               ranges::to_vector;
  path_info_ = l_tmp | ranges::views::filter([](path_info_t& in_info) -> bool {
                 auto l_ex = in_info.path_.extension();
                 return l_ex == ".ma" || l_ex == ".mb";
               }) |
               ranges::views::filter([](path_info_t& in_info) -> bool {
                 auto l_stem = in_info.path_.stem().generic_string();
                 return in_info.episode_.analysis(l_stem) && in_info.shot_.analysis(l_stem);
               }) |
               ranges::to_vector;
#ifdef DOODLE_USE_ENTT
  auto l_prj_view = g_reg()->view<assets, project>().each();
  if (ranges::distance(l_prj_view) != 0) {
    path_info_ |= ranges::actions::remove_if([&](path_info_t& in_info) -> bool {
      auto l_stem = in_info.path_.stem().generic_string();
      auto l_it   = ranges::find_if(l_prj_view, [&](const std::tuple<entt::entity, assets&, project&> in_tuple) {
        return l_stem.starts_with(std::get<project&>(in_tuple).p_shor_str);
      });
      if (l_it != l_prj_view.end()) {
        in_info.project_ = std::get<project&>(*l_it);
        return false;
      }
      return true;
    });
  }
#else
  auto l_prj_list = register_file_type::get_project_list();
  path_info_ |= ranges::actions::remove_if([&](path_info_t& in_info) -> bool {
    auto l_stem = in_info.path_.stem().generic_string();
    auto l_it   =
        ranges::find_if(l_prj_list, [&](const project& in_prj) { return l_stem.starts_with(in_prj.p_shor_str); });
    if (l_it != l_prj_list.end()) {
      in_info.project_ = *l_it;
      return false;
    }
    return true;
  });
#endif
}

void maya_tool::init() {
  ptr_attr->scoped_connection_1 = g_ctx().get<core_sig>().project_end_open.connect([this]() {
    p_text = g_reg()->ctx().get<project_config::base_config>().vfx_cloth_sim_path.generic_string();
  });
  p_text = g_reg()->ctx().get<project_config::base_config>().vfx_cloth_sim_path.generic_string();
  if (!g_ctx().contains<maya_ctx>())
    g_ctx().emplace<maya_ctx>();
}

entt::handle maya_tool::analysis_path(const path_info_t& in_path) {
  entt::handle l_handle{*g_reg(), g_reg()->create()};
  l_handle.emplace<process_message>(in_path.path_.filename().generic_string());
  l_handle.emplace<episodes>(in_path.episode_);
  l_handle.emplace<shot>(in_path.shot_);
  l_handle.emplace<project>(in_path.project_);
  return l_handle;
}

bool maya_tool::render() {
  ImGui::Text("解算文件列表(将文件拖入此处)");
  auto* l_win_main = ImGui::GetCurrentWindow();
  if (auto l_drag          = dear::DragDropTargetCustom{l_win_main->ContentRegionRect, l_win_main->ID}) {
    if (const auto* l_data = ImGui::AcceptDragDropPayload(doodle::doodle_config::drop_imgui_id.data());
      l_data && l_data->IsDelivery()) {
      auto* l_list = static_cast<std::vector<FSys::path>*>(l_data->Data);
      set_path(*l_list);
    }
  }
  if (auto l_c = dear::Child{"##mt_file_list", ImVec2{-FLT_MIN, dear::ListBox::DefaultHeight()}}) {
    for (const auto& f : path_info_) {
      dear::Selectable(f.path_.generic_string());
    }
  }

  dear::Text(fmt::format("解算资产: {}", p_text));

  imgui::Checkbox("自动上传", &p_upload_files);
  dear::TreeNode{"解算设置"} && [this]() {
    imgui::Checkbox(*ptr_attr->replace_ref_file_, &ptr_attr->replace_ref_file_);
    imgui::Checkbox(*ptr_attr->sim_file_, &ptr_attr->sim_file_);
    imgui::Checkbox(*ptr_attr->export_abc_type_, &ptr_attr->export_abc_type_);
  };
  dear::TreeNode{"fbx导出设置"} && [&]() { imgui::Checkbox("直接加载所有引用", &p_use_all_ref); };
  dear::TreeNode{*ptr_attr->ref_attr.ref_attr} && [&]() {
    if (ImGui::Button(*ptr_attr->ref_attr.de_button_attr)) {
      ptr_attr->ref_attr.ref_attr.data.emplace_back();
    }

    for (auto&& l_i : ptr_attr->ref_attr.ref_attr()) {
      ImGui::InputText(*l_i.f_file_path_attr, &l_i.f_file_path_attr);
      ImGui::InputText(*l_i.to_file_path_attr, &l_i.to_file_path_attr);
      ImGui::SameLine();
      if (ImGui::Button(*l_i.de_button_attr)) {
        boost::asio::post(g_io_context(), [l_i, this]() {
          ptr_attr->ref_attr.ref_attr() |=
              ranges::actions::remove_if([&](const maya_tool_ns::maya_reference_info& i) -> bool { return l_i == i; });
        });
      }
    }
  };
  imgui::Checkbox(*ptr_attr->create_play_blast_, &ptr_attr->create_play_blast_);
#if defined DOODLE_MAYA_TOOL
  dear::TreeNode{*ptr_attr->convert_maya_id_attr} && [&]() {
    dear::Combo{*ptr_attr->save_maya_type_attr.gui_name, ptr_attr->save_maya_type_attr.show_id_attr.c_str()} && [&]() {
      static auto l_list = magic_enum::enum_names<maya_tool_ns::maya_type>();
      for (auto&& l_i : l_list) {
        if (ImGui::Selectable(l_i.data())) {
          ptr_attr->save_maya_type_attr.show_id_attr = fmt::format(".{}", ptr_attr->save_maya_type_attr.show_id_attr);
          ptr_attr->save_maya_type_attr.data         = *magic_enum::enum_cast<maya_tool_ns::maya_type>(l_i);
        }
      }
    };
  };
#endif

  if (imgui::Button("解算")) {
    auto l_maya = g_reg()->ctx().get<maya_exe_ptr>();
    std::for_each(path_info_.begin(), path_info_.end(), [this, l_maya](const path_info_t& in_path) {
      auto k_arg             = maya_exe_ns::qcloth_arg{};
      k_arg.file_path        = in_path.path_;
      k_arg.project_         = g_ctx().get<database_n::file_translator_ptr>()->get_project_path();
      k_arg.t_post           = g_reg()->ctx().get<project_config::base_config>().t_post;
      k_arg.export_anim_time = g_reg()->ctx().get<project_config::base_config>().export_anim_time;
      if (ptr_attr->replace_ref_file_) k_arg.bitset_ |= maya_exe_ns::flags::k_replace_ref_file;
      if (ptr_attr->sim_file_) k_arg.bitset_ |= maya_exe_ns::flags::k_sim_file;
      if (ptr_attr->export_abc_type_) k_arg.bitset_ |= maya_exe_ns::flags::k_export_abc_type;
      if (ptr_attr->create_play_blast_) k_arg.bitset_ |= maya_exe_ns::flags::k_create_play_blast;
      k_arg.sim_path_list = list_sim_file(in_path.project_);
      auto l_msg_handle   = entt::handle{*g_reg(), g_reg()->create()};
      l_msg_handle.emplace<process_message>(in_path.path_.filename().generic_string());
      l_maya->async_run_maya(l_msg_handle, k_arg, [=](boost::system::error_code in_code, maya_exe_ns::maya_out_arg) {
        if (in_code) {
          l_msg_handle.get<process_message>().set_state(process_message::state::fail);
        } else {
          l_msg_handle.get<process_message>().set_state(process_message::state::success);
        }
      });
    });
  }
  ImGui::SameLine();
  if (imgui::Button("fbx导出")) {
    auto l_maya = g_reg()->ctx().get<maya_exe_ptr>();

    for (auto&& i : path_info_) {
      auto k_arg             = maya_exe_ns::export_fbx_arg{};
      k_arg.file_path        = i.path_;
      k_arg.use_all_ref      = this->p_use_all_ref;
      k_arg.upload_file      = p_upload_files;
      k_arg.export_anim_time = g_reg()->ctx().get<project_config::base_config>().export_anim_time;
      k_arg.project_         = g_ctx().get<database_n::file_translator_ptr>()->get_project_path();
      if (ptr_attr->create_play_blast_) k_arg.bitset_ |= maya_exe_ns::flags::k_create_play_blast;
      auto l_msg_handle = entt::handle{*g_reg(), g_reg()->create()};
      auto&& l_msg      = l_msg_handle.emplace<process_message>(i.path_.filename().generic_string());
      boost::asio::co_spawn(
        g_io_context(), async_run_maya(std::make_shared<maya_exe_ns::export_fbx_arg>(k_arg), l_msg.logger()),
        boost::asio::detached
      );
    }
  }
  ImGui::SameLine();
  if (imgui::Button("引用文件替换")) {
    auto l_maya = g_reg()->ctx().get<maya_exe_ptr>();
    std::for_each(path_info_.begin(), path_info_.end(), [this, l_maya](const path_info_t& i) {
      auto k_arg      = maya_exe_ns::replace_file_arg{};
      k_arg.file_path = i.path_;
      k_arg.file_list = ptr_attr->ref_attr.ref_attr() |
                        ranges::views::transform(
                          [](const maya_tool_ns::maya_reference_info& in_info) -> std::pair<FSys::path, FSys::path> {
                            return {in_info.f_file_path_attr.data, in_info.to_file_path_attr.data};
                          }
                        ) |
                        ranges::to_vector;
      k_arg.project_         = g_ctx().get<database_n::file_translator_ptr>()->get_project_path();
      k_arg.t_post           = g_reg()->ctx().get<project_config::base_config>().t_post;
      k_arg.export_anim_time = g_reg()->ctx().get<project_config::base_config>().export_anim_time;

      auto l_msg_handle = entt::handle{*g_reg(), g_reg()->create()};
      l_msg_handle.emplace<process_message>(i.path_.filename().generic_string());
      l_maya->async_run_maya(l_msg_handle, k_arg, [=](boost::system::error_code in_code, maya_exe_ns::maya_out_arg) {
        if (in_code) {
          l_msg_handle.get<process_message>().set_state(process_message::state::fail);
        } else {
          l_msg_handle.get<process_message>().set_state(process_message::state::success);
        }
      });
    });
  }

  if (imgui::Button("使用ue输出排屏(本机)")) {
    auto l_maya = g_reg()->ctx().get<maya_exe_ptr>();
    std::for_each(path_info_.begin(), path_info_.end(), [this, l_maya](const path_info_t& i) {
      auto k_arg             = maya_exe_ns::export_fbx_arg{};
      k_arg.file_path        = i.path_;
      k_arg.use_all_ref      = this->p_use_all_ref;
      k_arg.upload_file      = p_upload_files;
      k_arg.export_anim_time = g_reg()->ctx().get<project_config::base_config>().export_anim_time;
      k_arg.project_         = g_ctx().get<database_n::file_translator_ptr>()->get_project_path();

      auto l_msg = analysis_path(i);
      down_auto_light_anim_file l_down_anim_file{l_msg};
      import_and_render_ue l_import_and_render_ue{l_msg};
      auto_light_render_video l_auto_light_render_video{l_msg};
      up_auto_light_anim_file l_up_auto_light_file{l_msg};
      l_up_auto_light_file.async_end(boost::asio::bind_executor(
        g_io_context(),
        [l_msg](boost::system::error_code in_error_code, std::filesystem::path in_path) {
          if (in_error_code) {
            l_msg.get<process_message>().set_state(process_message::state::fail);
            return;
          }
          l_msg.get<process_message>().logger()->log(
            log_loc(), level::level_enum::warn, "上传自动灯光文件成功  路径 {}", in_path
          );
          l_msg.get<process_message>().set_state(process_message::state::success);
        }
      ));
      l_auto_light_render_video.async_end(boost::asio::bind_executor(g_io_context(), std::move(l_up_auto_light_file)));
      l_import_and_render_ue.async_end(boost::asio::bind_executor(g_io_context(), std::move(l_auto_light_render_video))
      );
      l_down_anim_file.async_down_end(boost::asio::bind_executor(g_io_context(), std::move(l_import_and_render_ue)));

      l_maya->async_run_maya(l_msg, k_arg, boost::asio::bind_executor(g_io_context(), std::move(l_down_anim_file)));
    });
  }
  ImGui::SameLine();
  if (imgui::Button("使用ue输出排屏(本机)(解算版)")) {
    auto l_maya = g_reg()->ctx().get<maya_exe_ptr>();
    std::for_each(path_info_.begin(), path_info_.end(), [this, l_maya](const path_info_t& i) {
      auto k_arg             = maya_exe_ns::qcloth_arg{};
      k_arg.file_path        = i.path_;
      k_arg.project_         = g_ctx().get<database_n::file_translator_ptr>()->get_project_path();
      k_arg.t_post           = g_reg()->ctx().get<project_config::base_config>().t_post;
      k_arg.export_anim_time = g_reg()->ctx().get<project_config::base_config>().export_anim_time;
      k_arg.bitset_ |= maya_exe_ns::flags::k_export_abc_type;
      k_arg.bitset_ |= maya_exe_ns::flags::k_touch_sim_file;
      k_arg.bitset_ |= maya_exe_ns::flags::k_create_play_blast;
      k_arg.bitset_ |= maya_exe_ns::flags::k_export_anim_file;
      k_arg.sim_path_list = list_sim_file(i.project_);

      auto l_msg = analysis_path(i);
      down_auto_light_anim_file l_down_anim_file{l_msg};
      import_and_render_ue l_import_and_render_ue{l_msg};
      auto_light_render_video l_auto_light_render_video{l_msg};
      up_auto_light_anim_file l_up_auto_light_file{l_msg};

      l_up_auto_light_file.async_end(boost::asio::bind_executor(
        g_io_context(),
        [l_msg](boost::system::error_code in_error_code, std::filesystem::path in_path) {
          if (in_error_code) {
            l_msg.get<process_message>().set_state(process_message::state::fail);
            return;
          }
          l_msg.get<process_message>().logger()->log(
            log_loc(), level::level_enum::warn, "上传自动灯光文件成功  路径 {}", in_path
          );
          l_msg.get<process_message>().set_state(process_message::state::success);
        }
      ));
      l_auto_light_render_video.async_end(boost::asio::bind_executor(g_io_context(), std::move(l_up_auto_light_file)));
      l_import_and_render_ue.async_end(boost::asio::bind_executor(g_io_context(), std::move(l_auto_light_render_video))
      );
      l_down_anim_file.async_down_end(boost::asio::bind_executor(g_io_context(), std::move(l_import_and_render_ue)));

      l_maya->async_run_maya(l_msg, k_arg, boost::asio::bind_executor(g_io_context(), std::move(l_down_anim_file)));
    });
  }

  if (imgui::Button("使用ue输出排屏(远程)")) {
    std::string l_host_name = boost::asio::ip::host_name();
    l_host_name             =
        ranges::views::all(l_host_name) |
        ranges::views::transform([](char in_c) { return std::tolower(in_c, core_set::get_set().utf8_locale); }) |
        ranges::to<std::string>;

    auto l_user_name = get_user_name();
    l_user_name      =
        ranges::views::all(l_user_name) |
        ranges::views::transform([](char in_c) { return std::tolower(in_c, core_set::get_set().utf8_locale); }) |
        ranges::to<std::string>;
    std::vector<nlohmann::json> l_task_list{};
    for (auto&& l_info : path_info_) {
      auto l_json                                       = create_cgru_json();
      l_json["job"]["name"]                             = l_info.path_.stem().generic_string();
      l_json["job"]["user_name"]                        = l_user_name;
      l_json["job"]["host_name"]                        = l_host_name;
      l_json["job"]["blocks"][0]["tasks"][0]["command"] = fmt::format(
        "D:/doodle_exe/bin/doodle_auto_light_process.exe --animation --maya_file=\"{}\"",
        l_info.path_.generic_string()
      );
      l_task_list.push_back(l_json);
    }
    post_http_task(l_task_list);
  }
  ImGui::SameLine();
  if (imgui::Button("使用ue输出排屏(远程)(解算版)")) {
    auto l_host_name = boost::asio::ip::host_name();
    auto l_user_name = get_user_name();
    std::vector<nlohmann::json> l_task_list{};
    for (auto&& l_info : path_info_) {
      auto l_json                                       = create_cgru_json();
      l_json["job"]["name"]                             = l_info.path_.stem().generic_string();
      l_json["job"]["user_name"]                        = l_user_name;
      l_json["job"]["host_name"]                        = l_host_name;
      l_json["job"]["blocks"][0]["tasks"][0]["command"] = fmt::format(
        "D:/doodle_exe/bin/doodle_auto_light_process.exe cfx --animation --maya_file=\"{}\"",
        l_info.path_.generic_string()
      );
      l_task_list.push_back(l_json);
    }
    post_http_task(l_task_list);
  }

  if (ImGui::Button("打开监视器")) {
    open_mir();
  }
  return open;
}

#define DOODLE_USE_CGRU_EXE
#ifdef DOODLE_USE_CGRU_EXE
namespace {
/*
 * CGRU_LOCATION=E:\cgru
PATH=%CGRU_LOCATION%/dll;%CGRU_LOCATION%/bin;%CGRU_LOCATION%/software_setup/bin;%CGRU_LOCATION%/python;%CGRU_LOCATION%/afanasy/bin
CGRU_PYTHON=%CGRU_LOCATION%/lib/python
 PYTHONHOME=%CGRU_LOCATION%\python
PYTHONPATH=%CGRU_LOCATION%/lib/python;%CGRU_LOCATION%/afanasy/python
python=%CGRU_LOCATION%/python
CGRU_PYTHONDIR=%CGRU_LOCATION%/python
CGRU_PYTHONEXE=python
rem chcp 65001
AF_ROOT=%CGRU_LOCATION%/afanasy
AF_PYTHON=%CGRU_LOCATION%/afanasy/python
AF_USERNAME=%USERNAME%
AF_HOSTNAME=%COMPUTERNAME%
 CGRU_VERSION=3.3.1
 */
boost::process::environment create_cgru_env() {
  std::string l_host_name = boost::asio::ip::host_name();
  l_host_name             =
      ranges::views::all(l_host_name) |
      ranges::views::transform([](char in_c) { return std::tolower(in_c, core_set::get_set().utf8_locale); }) |
      ranges::to<std::string>;

  auto l_user_name = get_user_name();
  l_user_name      =
      ranges::views::all(l_user_name) |
      ranges::views::transform([](char in_c) { return std::tolower(in_c, core_set::get_set().utf8_locale); }) |
      ranges::to<std::string>;
  boost::process::environment l_env = boost::this_process::environment();
  auto l_cgru                       = register_file_type::program_location().parent_path() / "cgru";

  auto l_cgru_version = l_cgru / "version.txt";

  l_env["CGRU_LOCATION"] = l_cgru.generic_string();
  l_env["Path"]          = fmt::format(
    "{};{};{};{};{}", l_cgru / "dll", l_cgru / "bin", l_cgru / "software_setup" / "bin", l_cgru / "python",
    l_cgru / "afanasy/bin"
  );
  l_env["CGRU_PYTHON"] = (l_cgru / "lib" / "python").generic_string();
  //  l_env["PYTHONHOME"]     = (l_cgru / "python").generic_string();
  l_env["PYTHONPATH"]     = fmt::format("{};{}", l_cgru / "lib" / "python", l_cgru / "afanasy/python");
  l_env["python"]         = (l_cgru / "python").generic_string();
  l_env["CGRU_PYTHONDIR"] = (l_cgru / "python").generic_string();
  l_env["CGRU_PYTHONEXE"] = "python";
  l_env["AF_ROOT"]        = (l_cgru / "afanasy").generic_string();
  l_env["AF_PYTHON"]      = (l_cgru / "afanasy" / "python").generic_string();
  l_env["AF_USERNAME"]    = l_user_name;
  l_env["AF_HOSTNAME"]    = l_host_name;

  if (FSys::exists(l_cgru_version)) {
    std::ifstream l_file{l_cgru_version.generic_string()};
    std::string l_version;
    std::getline(l_file, l_version);
    l_env["CGRU_VERSION"] = l_version;
  }

  // auto l_qt_conf = l_cgru / "afanasy" / "bin" / "qt.conf";
  // if (!FSys::exists(l_qt_conf)) {
  //   FSys::ofstream l_qt_conf_stream{l_qt_conf};
  //   l_qt_conf_stream << fmt::format("[Paths]\nPrefix = {}\n", (l_cgru / "dll").generic_string());
  // }
  return l_env;
}
} // namespace
#endif

void maya_tool::post_http_task(const std::vector<nlohmann::json>& in_task) {
  auto l_path = core_set::get_set().get_cache_root("cgru");
#ifdef DOODLE_USE_CGRU_EXE
  auto l_cgru = register_file_type::program_location().parent_path() / "cgru" / "afanasy" / "bin" / "afcmd.exe";
  auto l_env  = create_cgru_env();

  for (auto&& i : in_task) {
    auto l_json_path = FSys::write_tmp_file("cgru", i.dump(), ".json");
    auto l_args      = fmt::format(R"("{}" json send {})", l_cgru.generic_string(), l_json_path.generic_string());
    boost::process::async_system(
      g_io_context(),
      [](boost::system::error_code in_code, int in_exit) {
        if (in_code || in_exit) {
          default_logger_raw()->error("cgru afcmd.exe error code {} exit {}", in_code, in_exit);
        }
      },
      boost::process::cmd = l_args, boost::process::start_dir = l_cgru.parent_path().generic_string(),
      boost::process::env = l_env, boost::process::windows::hide,
      boost::process::std_out > (l_path / "cgru_out.txt").generic_string(),
      boost::process::std_err > (l_path / "cgru_err.txt").generic_string()
    );
  }

#else

  auto l_cmd  = boost::process::search_path("cmd.exe");
  auto l_cgru = register_file_type::program_location().parent_path() / "cgru" / "start" / "AFANASY" / "_afcmd.cmd";
  for (auto&& i : in_task) {
    auto l_json_path = FSys::write_tmp_file("cgru", i.dump(), ".json");
    boost::process::async_system(
        g_io_context(), [](boost::system::error_code, int) {}, boost::process::exe = l_cmd.generic_string(),
        boost::process::args = fmt::format(
            R"(/c ""{}" json send "{}"")", l_cgru.make_preferred().string(), l_json_path.make_preferred().string()
        ),
        //                 boost::process::windows::hide
        boost::process::std_out > (l_path / "cgru_out.txt").generic_string(),
        boost::process::std_err > (l_path / "cgru_err.txt").generic_string()
    );
  }
  auto l_afwatch =
      register_file_type::program_location().parent_path() / "cgru" / "start" / "AFANASY" / "10.afwatch.cmd";
  boost::process::async_system(
      g_io_context(), [](boost::system::error_code, int) {}, boost::process::exe = l_cmd.generic_string(),
      boost::process::args = fmt::format(R"(/c "{}")", l_afwatch.make_preferred().string()),
      boost::process::std_out > (l_path / "cgru_gui_out.txt").generic_string(),
      boost::process::std_err > (l_path / "cgru_gui_err.txt").generic_string()
  );
#endif
}

void maya_tool::open_mir() {
  auto l_env     = create_cgru_env();
  auto l_afwatch = register_file_type::program_location().parent_path() / "cgru" / "afanasy" / "bin" / "afwatch.exe";
  boost::process::async_system(
    g_io_context(),
    [](boost::system::error_code in_code, int in_exit) {
      if (in_code || in_exit) {
        default_logger_raw()->error("cgru afcmd.exe error code {} exit {}", in_code, in_exit);
      }
    },
    boost::process::start_dir = l_afwatch.parent_path().generic_string(),
    boost::process::exe       = l_afwatch.generic_string(), boost::process::env = l_env
  );
}

std::set<FSys::path> maya_tool::list_sim_file(const doodle::project& in_project) {
  auto l_sim_path = in_project.p_path / "6-moxing" / "CFX";

  std::set<FSys::path> l_ret{};
  for (auto&& l_i : FSys::directory_iterator{l_sim_path}) {
    if (l_i.is_regular_file() && l_i.path().extension() == ".ma") {
      l_ret.insert(l_i.path());
    }
  }
  return l_ret;
};

const std::string& maya_tool::title() const { return title_name_; }

maya_tool::~maya_tool() = default;
} // namespace doodle::gui