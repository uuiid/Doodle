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

#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/up_auto_light_file.h>
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>
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

namespace maya_tool_ns {
enum class maya_type { ma, mb };

class maya_file_type_gui : public gui_cache<maya_type> {
 public:
  maya_file_type_gui() : gui_cache<maya_type>("转换文件格式", maya_type::mb) {}

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
}  // namespace maya_tool_ns

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
  if (!g_ctx().contains<maya_ctx>()) g_ctx().emplace<maya_ctx>();
  if (!g_ctx().contains<ue_ctx>()) g_ctx().emplace<ue_ctx>();
  title_name_ = std::string{name};
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

  auto l_prj_list = register_file_type::get_project_list();
  path_info_ |= ranges::actions::remove_if([&](path_info_t& in_info) -> bool {
    auto l_stem = in_info.path_.stem().generic_string();
    auto l_it =
        ranges::find_if(l_prj_list, [&](const project& in_prj) { return l_stem.starts_with(in_prj.p_shor_str); });
    if (l_it != l_prj_list.end()) {
      in_info.project_ = *l_it;
      return false;
    }
    return true;
  });
}

bool maya_tool::render() {
  ImGui::Text("解算文件列表(将文件拖入此处)");
  auto* l_win_main = ImGui::GetCurrentWindow();
  if (auto l_drag = dear::DragDropTargetCustom{l_win_main->ContentRegionRect, l_win_main->ID}) {
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
    for (auto&& i : path_info_) {
      auto k_arg             = maya_exe_ns::qcloth_arg{};
      k_arg.file_path        = i.path_;
      k_arg.project_         = g_ctx().get<database_n::file_translator_ptr>()->get_project_path();
      k_arg.t_post           = g_reg()->ctx().get<project_config::base_config>().t_post;
      k_arg.export_anim_time = g_reg()->ctx().get<project_config::base_config>().export_anim_time;
      if (ptr_attr->replace_ref_file_) k_arg.bitset_ |= maya_exe_ns::flags::k_replace_ref_file;
      if (ptr_attr->sim_file_) k_arg.bitset_ |= maya_exe_ns::flags::k_sim_file;
      if (ptr_attr->export_abc_type_) k_arg.bitset_ |= maya_exe_ns::flags::k_export_abc_type;
      if (ptr_attr->create_play_blast_) k_arg.bitset_ |= maya_exe_ns::flags::k_create_play_blast;
      k_arg.sim_path    = i.project_.p_path / "6-moxing" / "CFX";
      auto l_msg_handle = entt::handle{*g_reg(), g_reg()->create()};
      auto&& l_msg      = l_msg_handle.emplace<process_message>(i.path_.filename().generic_string());
      boost::asio::co_spawn(
          g_thread(), async_run_maya(std::make_shared<maya_exe_ns::qcloth_arg>(k_arg), l_msg.logger()),
          boost::asio::bind_cancellation_slot(
              l_msg.get_cancel_slot(),
              [h = l_msg_handle](
                  std::exception_ptr, std::tuple<boost::system::error_code, maya_exe_ns::maya_out_arg> in_t
              ) {
                if (!std::get<0>(in_t))
                  h.get<process_message>().logger()->log(
                      level::off, magic_enum::enum_name(process_message::state::success)
                  );
              }
          )
      );
    }
  }
  ImGui::SameLine();
  if (imgui::Button("fbx导出")) {
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
          g_thread(), async_run_maya(std::make_shared<maya_exe_ns::export_fbx_arg>(k_arg), l_msg.logger()),
          boost::asio::bind_cancellation_slot(
              l_msg.get_cancel_slot(),
              [h = l_msg_handle](
                  std::exception_ptr, std::tuple<boost::system::error_code, maya_exe_ns::maya_out_arg> in_t
              ) {
                if (!std::get<0>(in_t))
                  h.get<process_message>().logger()->log(
                      level::off, magic_enum::enum_name(process_message::state::success)
                  );
              }
          )
      );
    }
  }
  ImGui::SameLine();
  if (imgui::Button("引用文件替换")) {
    for (auto&& i : path_info_) {
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

      auto l_msg_handle      = entt::handle{*g_reg(), g_reg()->create()};
      auto&& l_msg           = l_msg_handle.emplace<process_message>(i.path_.filename().generic_string());
      boost::asio::co_spawn(
          g_thread(), async_run_maya(std::make_shared<maya_exe_ns::replace_file_arg>(k_arg), l_msg.logger()),
          boost::asio::bind_cancellation_slot(
              l_msg.get_cancel_slot(),
              [h = l_msg_handle](
                  std::exception_ptr, std::tuple<boost::system::error_code, maya_exe_ns::maya_out_arg> in_t
              ) {
                if (!std::get<0>(in_t))
                  h.get<process_message>().logger()->log(
                      level::off, magic_enum::enum_name(process_message::state::success)
                  );
              }
          )
      );
    }
  }

  if (imgui::Button("使用ue输出排屏(本机)")) {
    for (auto&& i : path_info_) {
      auto k_arg             = maya_exe_ns::export_fbx_arg{};
      k_arg.file_path        = i.path_;
      k_arg.use_all_ref      = this->p_use_all_ref;
      k_arg.upload_file      = p_upload_files;
      k_arg.export_anim_time = g_reg()->ctx().get<project_config::base_config>().export_anim_time;
      k_arg.project_         = g_ctx().get<database_n::file_translator_ptr>()->get_project_path();
      auto l_msg_handle      = entt::handle{*g_reg(), g_reg()->create()};
      auto&& l_msg           = l_msg_handle.emplace<process_message>(i.path_.filename().generic_string());
      import_and_render_ue_ns::args l_args{};
      l_args.episodes_ = i.episode_;
      l_args.project_  = i.project_;
      l_args.shot_     = i.shot_;
      l_args.maya_arg_ = std::make_shared<maya_exe_ns::export_fbx_arg>(k_arg);
      boost::asio::co_spawn(
          g_thread(),
          async_auto_loght(std::make_shared<import_and_render_ue_ns::args>(std::move(l_args)), l_msg.logger()),
          boost::asio::bind_cancellation_slot(
              l_msg.get_cancel_slot(),
              [h = l_msg_handle](std::exception_ptr, std::tuple<boost::system::error_code, FSys::path> in_t) {
                if (!std::get<0>(in_t))
                  h.get<process_message>().logger()->log(
                      level::off, magic_enum::enum_name(process_message::state::success)
                  );
              }
          )
      );
    }
  }
  ImGui::SameLine();
  if (imgui::Button("使用ue输出排屏(本机)(解算版)")) {
    for (auto&& i : path_info_) {
      auto k_arg             = maya_exe_ns::qcloth_arg{};
      k_arg.file_path        = i.path_;
      k_arg.project_         = g_ctx().get<database_n::file_translator_ptr>()->get_project_path();
      k_arg.t_post           = g_reg()->ctx().get<project_config::base_config>().t_post;
      k_arg.export_anim_time = g_reg()->ctx().get<project_config::base_config>().export_anim_time;
      k_arg.bitset_ |= maya_exe_ns::flags::k_export_abc_type;
      k_arg.bitset_ |= maya_exe_ns::flags::k_touch_sim_file;
      k_arg.bitset_ |= maya_exe_ns::flags::k_create_play_blast;
      k_arg.bitset_ |= maya_exe_ns::flags::k_export_anim_file;
      k_arg.sim_path    = i.project_.p_path / "6-moxing" / "CFX";

      auto l_msg_handle = entt::handle{*g_reg(), g_reg()->create()};
      auto&& l_msg      = l_msg_handle.emplace<process_message>(i.path_.filename().generic_string());
      import_and_render_ue_ns::args l_args{};
      l_args.episodes_ = i.episode_;
      l_args.project_  = i.project_;
      l_args.shot_     = i.shot_;
      l_args.maya_arg_ = std::make_shared<maya_exe_ns::qcloth_arg>(k_arg);
      boost::asio::co_spawn(
          g_thread(),
          async_auto_loght(std::make_shared<import_and_render_ue_ns::args>(std::move(l_args)), l_msg.logger()),
          boost::asio::bind_cancellation_slot(
              l_msg.get_cancel_slot(),
              [h = l_msg_handle](std::exception_ptr, std::tuple<boost::system::error_code, FSys::path> in_t) {
                if (!std::get<0>(in_t))
                  h.get<process_message>().logger()->log(
                      level::off, magic_enum::enum_name(process_message::state::success)
                  );
              }
          )
      );
    }
  }

  if (imgui::Button("使用ue输出排屏(远程)")) {
    if (!p_render_client) p_render_client = std::make_shared<render_client::client>(core_set::get_render_url());
    for (auto&& i : path_info_)
      boost::asio::co_spawn(
          g_io_context(),
          p_render_client->render(
              i.path_.filename().generic_string(), "doodle_auto_light_process.exe",
              {fmt::format("--maya_file={}", i.path_), "--animation"}
          ),
          boost::asio::detached
      );
  }
  ImGui::SameLine();
  if (imgui::Button("使用ue输出排屏(远程)(解算版)")) {
    if (!p_render_client) p_render_client = std::make_shared<render_client::client>(core_set::get_render_url());
    for (auto&& i : path_info_)
      boost::asio::co_spawn(
          g_io_context(),
          p_render_client->render(
              i.path_.filename().generic_string(), "doodle_auto_light_process.exe",
              {fmt::format("--maya_file={}", i.path_), "--cfx"}
          ),
          boost::asio::detached
      );
  }

  if (ImGui::Button("打开监视器")) {
    open_mir();
  }
  return open;
}

void maya_tool::post_http_task(const std::vector<nlohmann::json>& in_task) {}

void maya_tool::open_mir() {}

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