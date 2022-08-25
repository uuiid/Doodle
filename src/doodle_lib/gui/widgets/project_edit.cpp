//
// Created by TD on 2022/2/7.
//

#include "project_edit.h"
#include <doodle_core/metadata/project.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_lib/gui/gui_ref/database_edit.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/init_register.h>

namespace doodle::gui {

class project_edit::impl {
  struct icon_extensions {
    gui::gui_cache_name_id name;
    gui::gui_cache_name_id button_name;
    std::vector<std::pair<gui::gui_cache<std::string>, gui_cache<bool>>> list;
    icon_extensions()
        : name("后缀名列表"s),
          button_name("添加"s),
          list() {}
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
  gui_cache_name_id list_name{"分类列表"s};
  gui_cache_name_id add_list_name_button{"添加"s};
  std::vector<std::pair<gui_cache<std::string>, gui_cache<bool>>> assets_list;
  std::string err_str;

  icon_extensions icon_list;
  gui_cache<std::int32_t> season_count{"季数计数", 20};

  std::vector<boost::signals2::scoped_connection> scoped_connections_;

  void config_init() {
    auto&& l_prj         = g_reg()->ctx().at<project>();
    project_name         = l_prj.get_name();
    project_path         = fmt::format("项目路径: {}", l_prj.get_path());

    auto& l_config       = g_reg()->ctx().at<project_config::base_config>();
    path_                = l_config.vfx_cloth_sim_path.generic_string();
    ue4_name             = l_config.export_group;
    cloth_proxy_         = l_config.cloth_proxy_;
    simple_module_proxy_ = l_config.simple_module_proxy_;

    regex_               = l_config.find_icon_regex;
    assets_list          = l_config.assets_list |
                  ranges::views::transform(
                      [](const std::string& in_str) {
                        return std::make_pair(gui_cache<std::string>{""s, in_str}, gui_cache<bool>{"删除", false});
                      }
                  ) |
                  ranges::to_vector;

    icon_list.list = l_config.icon_extensions |
                     ranges::views::transform(
                         [](const std::string& in_str) {
                           return std::make_pair(gui_cache<std::string>{""s, in_str}, gui_cache<bool>{"删除", false});
                         }
                     ) |
                     ranges::to_vector;
    upload_path  = l_config.upload_path.generic_string();
    season_count = l_config.season_count;
  }

  project_config::base_config get_config_() {
    project_config::base_config l_c{};
    l_c.vfx_cloth_sim_path   = path_.data;
    l_c.export_group         = ue4_name;
    l_c.cloth_proxy_         = cloth_proxy_;
    l_c.simple_module_proxy_ = simple_module_proxy_;
    l_c.find_icon_regex      = regex_;
    l_c.assets_list =
        assets_list |
        ranges::views::transform(
            [](const decltype(p_i->assets_list)::value_type& in_part) -> std::string {
              return in_part.first.data;
            }
        ) |
        ranges::to_vector;

    l_c.icon_extensions =
        icon_list.list |
        ranges::views::transform(
            [](const decltype(p_i->icon_list.list)::value_type& in_part) -> std::string {
              return in_part.first.data;
            }
        ) |
        ranges::to_vector;
    l_c.upload_path  = upload_path.data;
    l_c.season_count = season_count;
    return l_c;
  }
};

project_edit::project_edit()
    : p_i(std::make_unique<impl>()) {
  title_name_ = std::string{name};
}
project_edit::~project_edit() = default;

void project_edit::init() {
  p_i->config_init();
  p_i->scoped_connections_.emplace_back(
      g_reg()->ctx().at<core_sig>().project_end_open.connect(
          [this]() {
            p_i->config_init();
          }
      )
  );
  p_i->scoped_connections_.emplace_back(
      g_reg()->ctx().at<core_sig>().save.connect(1, [this]() {
        g_reg()->ctx().at<project_config::base_config>() = p_i->get_config_();
        g_reg()->ctx().at<project>().set_name(p_i->project_name.data);
      })
  );
}

void project_edit::render() {
  ImGui::Text("项目配置");
  imgui::InputText(*p_i->project_name.gui_name, &(p_i->project_name.data));
  dear::Text(p_i->project_path);

  ImGui::Text("基本配置:");

  imgui::InputText(*p_i->path_.gui_name, &(p_i->path_.data));
  imgui::InputText(*p_i->ue4_name.gui_name, &(p_i->ue4_name.data));
  imgui::InputText(*p_i->cloth_proxy_.gui_name, &(p_i->cloth_proxy_.data));
  imgui::InputText(*p_i->simple_module_proxy_.gui_name, &(p_i->simple_module_proxy_.data));

  ImGui::Text("其他配置:");
  /// @brief 正则表达式编辑
  if (ImGui::InputText(*p_i->regex_.gui_name, &p_i->regex_.data, ImGuiInputTextFlags_EnterReturnsTrue)) {
    try {
      std::regex l_regex{p_i->regex_.data};

      p_i->err_str.clear();
    } catch (const std::regex_error& error) {
      p_i->err_str = fmt::format("错误的正则表达式 {}", p_i->regex_.data);
      DOODLE_LOG_ERROR(p_i->err_str)
    }
  };
  if (!p_i->err_str.empty()) {
    dear::Text(p_i->err_str);
  }
  /// @brief 添加分类编辑
  if (ImGui::Button(*p_i->add_list_name_button)) {
    p_i->assets_list.emplace_back(std::make_pair(gui_cache<std::string>{""s, "null"s}, gui_cache<bool>{"删除", false}));
  }
  dear::ListBox{*p_i->list_name} && [&]() {
    for (auto&& i : p_i->assets_list) {
      if (ImGui::InputText(*i.first.gui_name, &i.first.data))

        ImGui::SameLine();
      if (ImGui::Button(*i.second.gui_name)) {
        i.second.data = true;
      }
    }
  };
  const auto l_r =
      ranges::remove_if(p_i->assets_list, [](const decltype(p_i->assets_list)::value_type& in_part) -> bool {
        return in_part.second.data;
      });
  if (l_r != p_i->assets_list.end())
    p_i->assets_list.erase(l_r, p_i->assets_list.end());
  /// @brief 后缀名编辑
  if (ImGui::Button(*p_i->icon_list.button_name)) {
    p_i->icon_list.list.emplace_back(std::make_pair(gui_cache<std::string>{""s, "null"s}, gui_cache<bool>{"删除", false}));
  }
  dear::ListBox{*p_i->icon_list.name} && [&]() {
    for (auto&& i : p_i->icon_list.list) {
      if (ImGui::InputText(*i.first.gui_name, &i.first.data))

        ImGui::SameLine();
      if (ImGui::Button(*i.second.gui_name)) {
        i.second.data = true;
      }
    }
  };
  const auto l_r_ =
      ranges::remove_if(p_i->icon_list.list, [](const decltype(p_i->icon_list.list)::value_type& in_part) -> bool {
        return in_part.second.data;
      });
  if (l_r_ != p_i->icon_list.list.end())
    p_i->icon_list.list.erase(l_r_, p_i->icon_list.list.end());

  ImGui::InputText(*p_i->upload_path.gui_name, &p_i->upload_path.data);
  ImGui::InputInt(*p_i->season_count.gui_name, &p_i->season_count.data);

  if (ImGui::Button("保存"))
    g_reg()->ctx().at<core_sig>().save();
}
}  // namespace doodle::gui
