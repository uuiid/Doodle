//
// Created by TD on 2022/9/21.
//

#include "maya_tool.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/redirection_path_info.h>
#include <doodle_core/metadata/season.h>
#include <doodle_core/metadata/shot.h>

#include <doodle_app/gui/base/ref_base.h>

#include <doodle_lib/exe_warp/maya_exe.h>

#include <utility>

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

  entt::handle save_handle{make_handle()};

  void set_value() {
    auto& l_ass  = save_handle.get_or_emplace<assets_file>();
    auto& l_path = save_handle.get_or_emplace<redirection_path_info>();

    l_ass.path_attr(f_file_path_attr());
    FSys::path l_f{to_file_path_attr()};
    l_path.file_name_   = l_f.filename();
    l_path.search_path_ = {l_f.parent_path()};
  }

  bool operator==(const maya_reference_info& in_r) const {
    return std::tie(f_file_path_attr, to_file_path_attr) == std::tie(in_r.f_file_path_attr, in_r.to_file_path_attr);
  }
};

class ref_attr_gui {
 public:
  ref_attr_gui()  = default;
  ~ref_attr_gui() = default;
  gui_cache<std::vector<maya_tool_ns::maya_reference_info>> ref_attr{
      "替换文件设置"s, std::vector<maya_tool_ns::maya_reference_info>{}};
  gui_cache_name_id de_button_attr{"添加"};
};

}  // namespace maya_tool_ns

class maya_tool::impl {
 public:
  gui_cache_name_id convert_maya_id_attr{"转换文件设置"};

  maya_tool_ns::maya_file_type_gui save_maya_type_attr{};
  maya_tool_ns::ref_attr_gui ref_attr{};
};

maya_tool::maya_tool()
    : p_cloth_path(),
      p_text(),
      p_sim_path(),
      p_only_sim(false),
      p_use_all_ref(false),
      p_upload_files(false),
      p_sim_export_fbx(true),
      p_sim_only_export(),
      ptr_attr(std::make_unique<impl>()) {
  g_reg()->ctx().emplace<maya_exe_ptr>() = std::make_shared<maya_exe>();
  title_name_                            = std::string{name};
}
void maya_tool::init() {
  g_reg()->ctx().at<core_sig>().project_end_open.connect([this]() {
    p_text = g_reg()->ctx().at<project_config::base_config>().vfx_cloth_sim_path.generic_string();
  });
  g_reg()->ctx().at<core_sig>().select_handles.connect([this](const std::vector<entt::handle>& in_list) {
    p_sim_path = in_list | ranges::views::filter([](const entt::handle& in_handle) -> bool {
                   return in_handle && in_handle.any_of<assets_file>();
                 }) |
                 ranges::views::filter([](const entt::handle& in_handle) -> bool {
                   auto l_ex = in_handle.get<assets_file>().path_attr().extension();
                   return l_ex == ".ma" || l_ex == ".mb";
                 }) |
                 ranges::views::transform([](const entt::handle& in_handle) -> FSys::path {
                   return in_handle.get<assets_file>().get_path_normal();
                 }) |
                 ranges::to_vector;
  });

  p_text = g_reg()->ctx().at<project_config::base_config>().vfx_cloth_sim_path.generic_string();
  g_reg()->ctx().emplace<maya_tool&>(*this);
}

void maya_tool::render() {
  dear::ListBox{"file_list"} && [this]() {
    for (const auto& f : p_sim_path) {
      dear::Selectable(f.generic_string());
    }
  };

  dear::Text(fmt::format("解算资产: {}", p_text));

  imgui::Checkbox("自动上传", &p_upload_files);
  dear::TreeNode{"解算设置"} && [this]() {
    imgui::Checkbox("只解算不替换引用", &p_only_sim);
    imgui::Checkbox("导出为fbx格式", &p_sim_export_fbx);
    imgui::Checkbox("只导出", &p_sim_only_export);
  };
  dear::TreeNode{"fbx导出设置"} && [&]() { imgui::Checkbox("直接加载所有引用", &p_use_all_ref); };
  dear::TreeNode{*ptr_attr->ref_attr.ref_attr} && [&]() {
    if (ImGui::Button(*ptr_attr->ref_attr.de_button_attr)) {
      ptr_attr->ref_attr.ref_attr.data.emplace_back();
    }

    for (auto&& l_i : ptr_attr->ref_attr.ref_attr()) {
      if (ImGui::InputText(*l_i.f_file_path_attr, &l_i.f_file_path_attr)) l_i.set_value();
      if (ImGui::InputText(*l_i.to_file_path_attr, &l_i.to_file_path_attr)) l_i.set_value();
      ImGui::SameLine();
      if (ImGui::Button(*l_i.de_button_attr)) {
        boost::asio::post(g_io_context(), [l_i, this]() {
          ptr_attr->ref_attr.ref_attr() |=
              ranges::actions::remove_if([&](const maya_tool_ns::maya_reference_info& i) -> bool { return l_i == i; });
        });
      }
    }
  };

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

  if (imgui::Button("解算")) {
    auto l_maya = g_reg()->ctx().at<maya_exe_ptr>();
    std::for_each(p_sim_path.begin(), p_sim_path.end(), [this, l_maya](const FSys::path& in_path) {
      auto k_arg             = maya_exe_ns::qcloth_arg{};
      k_arg.file_path        = in_path;
      k_arg.only_sim         = p_only_sim;
      k_arg.upload_file      = p_upload_files;
      k_arg.export_fbx       = p_sim_export_fbx;
      k_arg.only_export      = p_sim_only_export;
      k_arg.project_         = g_reg()->ctx().at<database_info>().path_;
      k_arg.t_post           = g_reg()->ctx().at<project_config::base_config>().t_post;
      k_arg.export_anim_time = g_reg()->ctx().at<project_config::base_config>().export_anim_time;
      l_maya->async_run_maya(make_handle(), k_arg, [](boost::system::error_code in_code) {
        if (in_code) DOODLE_LOG_ERROR(in_code);
        DOODLE_LOG_ERROR("完成任务");
      });
    });
  }
  ImGui::SameLine();
  if (imgui::Button("fbx导出")) {
    auto l_maya = g_reg()->ctx().at<maya_exe_ptr>();
    std::for_each(p_sim_path.begin(), p_sim_path.end(), [this, l_maya](const FSys::path& i) {
      auto k_arg             = maya_exe_ns::export_fbx_arg{};
      k_arg.file_path        = i;
      k_arg.use_all_ref      = this->p_use_all_ref;
      k_arg.upload_file      = p_upload_files;
      k_arg.t_post           = g_reg()->ctx().at<project_config::base_config>().t_post;
      k_arg.export_anim_time = g_reg()->ctx().at<project_config::base_config>().export_anim_time;

      k_arg.project_         = g_reg()->ctx().at<database_info>().path_;
      l_maya->async_run_maya(make_handle(), k_arg, [](boost::system::error_code in_code) {
        if (in_code) DOODLE_LOG_ERROR(in_code);
        DOODLE_LOG_ERROR("完成任务");
      });
    });
  }
  ImGui::SameLine();
  if (imgui::Button("引用文件替换")) {
    auto l_maya = g_reg()->ctx().at<maya_exe_ptr>();
    std::for_each(p_sim_path.begin(), p_sim_path.end(), [this, l_maya](const FSys::path& i) {
      auto k_arg             = maya_exe_ns::replace_file_arg{};
      k_arg.file_path        = i;
      k_arg.replace_file_all = true;
      k_arg.save_handle =
          ptr_attr->ref_attr.ref_attr() |
          ranges::views::transform([](const maya_tool_ns::maya_reference_info& in_info) -> entt::handle {
            return in_info.save_handle;
          }) |
          ranges::to_vector;
      k_arg.project_         = g_reg()->ctx().at<database_info>().path_;
      k_arg.t_post           = g_reg()->ctx().at<project_config::base_config>().t_post;
      k_arg.export_anim_time = g_reg()->ctx().at<project_config::base_config>().export_anim_time;

      l_maya->async_run_maya(make_handle(), k_arg, [](boost::system::error_code in_code) {
        if (in_code) DOODLE_LOG_ERROR(in_code);
        DOODLE_LOG_ERROR("完成任务");
      });
    });
  }
  ImGui::SameLine();
  if (ImGui::Button("转换格式")) {
    auto l_maya = g_reg()->ctx().at<maya_exe_ptr>();
    std::for_each(p_sim_path.begin(), p_sim_path.end(), [this, l_maya](const FSys::path& i) {
      auto k_arg                     = maya_exe_ns::clear_file_arg{};
      k_arg.file_path                = i;
      k_arg.project_                 = g_reg()->ctx().at<database_info>().path_;
      k_arg.t_post                   = g_reg()->ctx().at<project_config::base_config>().t_post;
      k_arg.export_anim_time         = g_reg()->ctx().at<project_config::base_config>().export_anim_time;
      k_arg.save_file_extension_attr = ptr_attr->save_maya_type_attr.show_id_attr;

      l_maya->async_run_maya(make_handle(), k_arg, [](boost::system::error_code in_code) {
        if (in_code) DOODLE_LOG_ERROR(in_code);
        DOODLE_LOG_ERROR("完成任务");
      });
    });
  }
}
const std::string& maya_tool::title() const { return title_name_; }

maya_tool::~maya_tool() = default;
}  // namespace doodle::gui
