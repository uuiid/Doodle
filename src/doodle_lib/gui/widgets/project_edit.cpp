//
// Created by TD on 2022/2/7.
//

#include "project_edit.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/metadata/user.h"
#include <doodle_core/core/core_sig.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/metadata/project.h>

#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/lib_warp/imgui_warp.h>

namespace doodle::gui {

namespace {
class camera_judge_gui : boost::equality_comparable<camera_judge_gui> {
 public:
  camera_judge_gui() = default;
  explicit camera_judge_gui(const project_config::camera_judge& in_judge)
      : camera_regex("正则表达式", in_judge.first), judge("判断优先级", in_judge.second) {}

  gui_cache<std::string> camera_regex{"正则表达式", ""s};
  gui_cache<std::int32_t> judge{"判断优先级", 100};
  gui_cache_name_id delete_button{"删除"s};
  explicit operator project_config::camera_judge() { return std::make_pair(camera_regex(), judge()); }

  bool operator==(const camera_judge_gui& in_l) const {
    return std::tie(camera_regex, judge, delete_button) == std::tie(in_l.camera_regex, in_l.judge, in_l.delete_button);
  }
};

class camera_judge_table_gui {
 public:
  std::vector<camera_judge_gui> camera_judge_list_gui{};
  gui_cache_name_id add_camera_judge_list_gui{"添加"};
  gui_cache_name_id table_name{"camera_judge_table_gui"};
};
}  // namespace

class project_edit::impl {
  struct icon_extensions {
    std::string name;
    gui::gui_cache_name_id button_name;
    std::vector<std::pair<gui::gui_cache<std::string>, gui_cache<bool>>> list;
    icon_extensions() : name("后缀名列表"s), button_name("添加"s), list() {}
  };

 public:
  std::string project_path{};
  gui::gui_cache<std::string> project_name{"名称"s, ""s};

  gui_cache<std::string> path_{"解算路径"s, ""s};
  gui_cache<std::string> ue4_name{"导出组名称"s, ""s};
  gui_cache<std::string> cloth_proxy_{"布料解算后缀名", ""s};
  gui_cache<std::string> simple_module_proxy_{"动画后缀名", ""s};

  gui_cache<std::string> regex_{"后缀名识别(正则表达式)"s, ""s};
  gui_cache<std::string> upload_path{"上传路径"s, ""s};
  std::string err_str;

  icon_extensions icon_list;
  gui_cache<std::int32_t> season_count{"季数计数", 20};

  gui_cache<bool> use_only_sim_cloth{"只导出解算模型"s, false};
  gui_cache<bool> use_divide_group_export{"使用组划分导出"s, false};

  gui_cache<std::int32_t> t_post{"TPost时间"s, 950};
  gui_cache<std::int32_t> export_anim_time{"导出动画开始帧"s, 1001};

  camera_judge_table_gui camera_judge_gui_attr{};
  gui_cache<std::string> abc_export_extract_reference_name{"重新提取引用名称"s, ""s};
  gui_cache<std::string> abc_export_format_reference_name{"重新格式化引用名称"s, ""s};

  gui_cache<std::string> abc_export_extract_scene_name{"重新提取文件名称"s, ""s};
  gui_cache<std::string> abc_export_format_scene_name{"重新提格式化件名称"s, ""s};

  gui_cache<bool> abc_export_add_frame_range{"导出附加范围"s, true};

  gui_cache<std::string> maya_camera_suffix{"相机后缀"s, "camera"s};
  gui_cache<std::string> maya_out_put_abc_suffix{"分组导出abc寻找后缀"s, "_output_abc"s};

  std::vector<boost::signals2::scoped_connection> scoped_connections_;
  std::string title_name_;
  bool open{true};

  void config_init() {
    auto&& l_prj         = g_reg()->ctx().get<project>();
    project_name         = l_prj.get_name();
    project_path         = fmt::format("项目路径: {}", l_prj.get_path());

    auto& l_config       = g_reg()->ctx().get<project_config::base_config>();
    path_                = l_config.vfx_cloth_sim_path.generic_string();
    ue4_name             = l_config.export_group;
    cloth_proxy_         = l_config.cloth_proxy_;
    simple_module_proxy_ = l_config.simple_module_proxy_;

    regex_               = l_config.find_icon_regex;

    icon_list.list       = l_config.icon_extensions | ranges::views::transform([](const std::string& in_str) {
                       return std::make_pair(gui_cache<std::string>{""s, in_str}, gui_cache<bool>{"删除", false});
                     }) |
                     ranges::to_vector;
    upload_path             = l_config.upload_path.generic_string();
    season_count            = l_config.season_count;

    use_only_sim_cloth      = l_config.use_only_sim_cloth;
    use_divide_group_export = l_config.use_divide_group_export;

    t_post                  = l_config.t_post;
    export_anim_time        = l_config.export_anim_time;

    camera_judge_gui_attr.camera_judge_list_gui =
        l_config.maya_camera_select |
        ranges::views::transform([](const project_config::camera_judge& in_camera_judge) -> camera_judge_gui {
          return camera_judge_gui{in_camera_judge};
        }) |
        ranges::to_vector;

    abc_export_extract_reference_name() = l_config.abc_export_extract_reference_name;
    abc_export_format_reference_name()  = l_config.abc_export_format_reference_name;
    abc_export_extract_scene_name()     = l_config.abc_export_extract_scene_name;
    abc_export_format_scene_name()      = l_config.abc_export_format_scene_name;
    abc_export_add_frame_range()        = l_config.abc_export_add_frame_range;
    maya_camera_suffix()                = l_config.maya_camera_suffix;
    maya_out_put_abc_suffix()           = l_config.maya_out_put_abc_suffix;
  }

  project_config::base_config get_config_() {
    project_config::base_config l_c{};
    l_c.vfx_cloth_sim_path   = path_.data;
    l_c.export_group         = ue4_name;
    l_c.cloth_proxy_         = cloth_proxy_;
    l_c.simple_module_proxy_ = simple_module_proxy_;
    l_c.find_icon_regex      = regex_;

    l_c.icon_extensions =
        icon_list.list |
        ranges::views::transform([](const decltype(p_i->icon_list.list)::value_type& in_part) -> std::string {
          return in_part.first.data;
        }) |
        ranges::to_vector;
    l_c.upload_path             = upload_path.data;
    l_c.season_count            = season_count;

    l_c.use_only_sim_cloth      = use_only_sim_cloth;
    l_c.use_divide_group_export = use_divide_group_export;

    l_c.t_post                  = t_post;
    l_c.export_anim_time        = export_anim_time;

    l_c.maya_camera_select      = camera_judge_gui_attr.camera_judge_list_gui |
                             ranges::views::transform([](auto&& in) -> project_config::camera_judge {
                               return project_config::camera_judge{in};
                             }) |
                             ranges::to_vector;
    l_c.abc_export_extract_reference_name = abc_export_extract_reference_name();
    l_c.abc_export_format_reference_name  = abc_export_format_reference_name();
    l_c.abc_export_extract_scene_name     = abc_export_extract_scene_name();
    l_c.abc_export_format_scene_name      = abc_export_format_scene_name();
    l_c.abc_export_add_frame_range        = abc_export_add_frame_range();
    l_c.maya_camera_suffix                = maya_camera_suffix();
    l_c.maya_out_put_abc_suffix           = maya_out_put_abc_suffix();

    return l_c;
  }
};

project_edit::project_edit() : p_i(std::make_unique<impl>()) {
  p_i->title_name_ = std::string{name};
  init();
}
project_edit::~project_edit() = default;

void project_edit::init() {
  p_i->config_init();
  p_i->scoped_connections_.emplace_back(g_ctx().get<core_sig>().project_end_open.connect([this]() {
    p_i->config_init();
  }));
}

bool project_edit::render() {
  ImGui::Text("项目配置");
  imgui::InputText(*p_i->project_name.gui_name, &(p_i->project_name.data));
  dear::Text(p_i->project_path);

  ImGui::Text("解算配置:");
  imgui::InputText(*p_i->path_.gui_name, &(p_i->path_.data));
  imgui::InputText(*p_i->ue4_name.gui_name, &(p_i->ue4_name.data));
  imgui::InputText(*p_i->cloth_proxy_.gui_name, &(p_i->cloth_proxy_.data));
  imgui::InputText(*p_i->simple_module_proxy_.gui_name, &(p_i->simple_module_proxy_.data));
  ImGui::Checkbox(*p_i->use_only_sim_cloth, &p_i->use_only_sim_cloth);
  ImGui::Checkbox(*p_i->use_divide_group_export, &p_i->use_divide_group_export);
  if (p_i->use_divide_group_export()) ImGui::InputText(*p_i->maya_out_put_abc_suffix, &p_i->maya_out_put_abc_suffix);

  ImGui::InputInt(*p_i->t_post, &p_i->t_post);
  ImGui::InputInt(*p_i->export_anim_time, &p_i->export_anim_time);

  ImGui::Text("相机优先级配置:");
  if (ImGui::Button(*p_i->camera_judge_gui_attr.add_camera_judge_list_gui)) {
    p_i->camera_judge_gui_attr.camera_judge_list_gui.emplace_back();
  }
  //,
  //      ImGuiTableFlags_::ImGuiTableFlags_SizingStretchSame
  dear::Table{*p_i->camera_judge_gui_attr.table_name, 3} && [&]() {
    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 400);
    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 220);
    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 40);

    ranges::for_each(p_i->camera_judge_gui_attr.camera_judge_list_gui, [this](camera_judge_gui& in) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::InputText(*in.camera_regex, &in.camera_regex);
      ImGui::TableNextColumn();
      ImGui::InputInt(*in.judge, &in.judge);
      ImGui::TableNextColumn();
      if (ImGui::Button(*in.delete_button))
        boost::asio::post(g_io_context(), [in, this]() {
          p_i->camera_judge_gui_attr.camera_judge_list_gui |=
              ranges::actions::remove_if([&](const camera_judge_gui& in_) -> bool { return in_ == in; });
        });
    });
  };

  imgui::InputText(*p_i->maya_camera_suffix, &(p_i->maya_camera_suffix));

  ImGui::Text("导出abc配置:");

  ImGui::InputText(*p_i->abc_export_extract_reference_name, &p_i->abc_export_extract_reference_name);
  ImGui::InputText(*p_i->abc_export_format_reference_name, &p_i->abc_export_format_reference_name);
  ImGui::InputText(*p_i->abc_export_extract_scene_name, &p_i->abc_export_extract_scene_name);
  ImGui::InputText(*p_i->abc_export_format_scene_name, &p_i->abc_export_format_scene_name);
  ImGui::Checkbox(*p_i->abc_export_add_frame_range, &p_i->abc_export_add_frame_range);

  ImGui::Text("其他配置:");
  /// @brief 正则表达式编辑
  if (ImGui::InputText(*p_i->regex_.gui_name, &p_i->regex_.data, ImGuiInputTextFlags_EnterReturnsTrue)) {
    try {
      std::regex l_regex{p_i->regex_.data};

      p_i->err_str.clear();
    } catch (const std::regex_error& error) {
      p_i->err_str = fmt::format("错误的正则表达式 {}", p_i->regex_.data);
      DOODLE_LOG_ERROR(p_i->err_str);
    }
  };
  if (!p_i->err_str.empty()) {
    dear::Text(p_i->err_str);
  }

  /// @brief 后缀名编辑
  dear::Text(p_i->icon_list.name);
  ImGui::SameLine();
  if (ImGui::Button(*p_i->icon_list.button_name)) {
    p_i->icon_list.list.emplace_back(
        std::make_pair(gui_cache<std::string>{""s, "null"s}, gui_cache<bool>{"删除", false})
    );
  }
  for (auto&& i : p_i->icon_list.list) {
    if (ImGui::InputText(*i.first.gui_name, &i.first.data))
      ;
    ImGui::SameLine();
    if (ImGui::Button(*i.second.gui_name)) {
      i.second.data = true;
    }
  }

  const auto l_r_ =
      ranges::remove_if(p_i->icon_list.list, [](const decltype(p_i->icon_list.list)::value_type& in_part) -> bool {
        return in_part.second.data;
      });
  if (l_r_ != p_i->icon_list.list.end()) p_i->icon_list.list.erase(l_r_, p_i->icon_list.list.end());

  ImGui::InputText(*p_i->upload_path.gui_name, &p_i->upload_path.data);
  ImGui::InputInt(*p_i->season_count.gui_name, &p_i->season_count.data);

  if (ImGui::Button("保存")) {
    save();
  }

  return p_i->open;
}

void project_edit::save() {
  g_reg()->ctx().get<user::current_user>().get_handle().patch<user>();
  g_reg()->ctx().get<project_config::base_config>() = p_i->get_config_();
  g_reg()->ctx().get<project>().set_name(p_i->project_name.data);
}

const std::string& project_edit::title() const { return p_i->title_name_; }
}  // namespace doodle::gui
