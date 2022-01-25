//
// Created by TD on 2021/9/18.
//

#include "command_meta.h"

#include <doodle_lib/core/doodle_lib.h>
#include <gui/open_file_dialog.h>
#include <doodle_lib/doodle_app.h>
#include <doodle_lib/gui/widgets/assets_widget.h>
#include <doodle_lib/gui/widgets/time_widget.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata_cpp.h>

#include <boost/range.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/irange.hpp>
namespace doodle {

class comm_project_add::impl {
 public:
  string p_prj_name;
  string p_prj_name_short;
  string p_prj_path;
  string vfx_cloth_sim_path;

  entt::handle p_root;

  project::cloth_config cloth_config;
  impl()
      : p_prj_name("none"),
        p_prj_name_short("none"),
        p_prj_path("C:/"),
        p_root() {
  }
};

comm_project_add::comm_project_add()
    : p_impl(std::make_unique<impl>()) {
  p_name     = "项目";
  p_show_str = make_imgui_name(this, "删除", "添加",
                               "修改", "名称",
                               "路径", "选择",
                               "解算路径",
                               "simple_subsampling",
                               "frame_samples",
                               "time_scale",
                               "length_scale",
                               "解算配置",
                               "布料代理",
                               "布料节点",
                               "导出节点"

  );
}

bool comm_project_add::render() {
  auto& k_d_lib = doodle_lib::Get();

  if (imgui::Button(p_show_str["添加"].c_str())) {
    auto k_en = make_handle();

    k_en.emplace<project>(p_impl->p_prj_path, fmt::format("{}_{}", p_impl->p_prj_name, g_reg()->view<project>().size()));
    k_en.get<root_ref>().set_root(k_en);
    k_en.patch<database>(database::save{});

    p_impl->p_root = k_en;
  }
  if (p_impl->p_root) {
    if (imgui::Button(p_show_str["修改"].c_str())) {
      p_impl->p_root.patch<database>(database::save{});
    }
    imgui::SameLine();
    if (imgui::Button(p_show_str["删除"].c_str())) {
      p_impl->p_root.patch<database>(database::save{});
    }
    if (imgui::InputText(p_show_str["名称"].c_str(), &(p_impl->p_prj_name))) {
      p_impl->p_root.get<project>().set_name(p_impl->p_prj_name);
    }
    if (imgui::InputText(p_show_str["路径"].c_str(), &(p_impl->p_prj_path))) {
      p_impl->p_root.get<project>().set_path(p_impl->p_prj_path);
    }
    imgui::SameLine();
    if (imgui::Button(p_show_str["选择"].c_str())) {
      g_main_loop().attach<file_dialog>(
          [this](const FSys::path& in) {
            p_impl->p_prj_path = in.generic_string();
          },
          "选择路径");
    }
  }

  if (!p_impl->p_root || !p_impl->p_root.any_of<project::cloth_config>())
    return true;
  dear::TreeNode{p_show_str["解算配置"].c_str()} && [&]() {
    if (imgui::InputText(p_show_str["解算路径"].c_str(), &(p_impl->vfx_cloth_sim_path))) {
      p_impl->p_root.patch<project::cloth_config>([&](project::cloth_config& in) {
        in.vfx_cloth_sim_path = p_impl->vfx_cloth_sim_path;
      });
    }

    if (imgui::InputText(p_show_str["导出节点"].c_str(), &(p_impl->cloth_config.export_group))) {
      p_impl->p_root.patch<project::cloth_config>([&](project::cloth_config& in) {
        in.export_group = p_impl->cloth_config.export_group;
      });
    }
  };

  return true;
}
bool comm_project_add::set_data(const entt::handle& in_data) {
  if (in_data.any_of<project>()) {
    p_impl->p_root = in_data;
    if (p_impl->p_root) {
      auto& k_prj                = p_impl->p_root.get<project>();
      p_impl->p_prj_name         = k_prj.get_name();
      p_impl->p_prj_path         = k_prj.get_path().generic_string();
      p_impl->cloth_config       = k_prj.get_vfx_cloth_config();
      p_impl->vfx_cloth_sim_path = p_impl->cloth_config.vfx_cloth_sim_path.generic_string();
    }
  } else {
    p_impl->p_root = entt::handle{};
  }
  return true;
}
comm_project_add::comm_project_add(const comm_project_add& in_add) noexcept
    : command_base(in_add),
      p_impl() {
  p_impl = std::make_unique<impl>(*(in_add.p_impl));
}
comm_project_add& comm_project_add::operator=(const comm_project_add& in_add) noexcept {
  p_impl = std::make_unique<impl>(*(in_add.p_impl));
  return *this;
}
comm_project_add::~comm_project_add() = default;

comm_ass_eps::comm_ass_eps()
    : p_root(),
      p_data(0) {
  p_name     = "集数";
  p_show_str = make_imgui_name(this, "添加",
                               "批量添加",
                               "修改",
                               "删除",
                               "集数",
                               "结束集数");
}

bool comm_ass_eps::render() {
  if (!p_root.all_of<episodes>()) {
    if (imgui::Button(p_show_str["添加"].c_str())) {
      p_root.emplace<episodes>(p_data);
      p_root.patch<database>(database::save{});
      p_root.get<root_ref>().set_root(g_reg()->ctx<root_ref>().root_handle());
    }
  } else {
    if (imgui::Button(p_show_str["修改"].c_str())) {
      p_root.patch<episodes>([&](auto& eps) { eps.set_episodes(p_data); });
      p_root.patch<database>(database::save{});
    }
    imgui::SameLine();
    if (imgui::Button(p_show_str["删除"].c_str())) {
      p_root.remove<episodes>();
      p_root.patch<database>(database::save{});
    }
  }
  imgui::InputInt(p_show_str["集数"].c_str(), &p_data, 1, 9999);
  return true;
}
bool comm_ass_eps::set_data(const entt::handle& in_data) {
  p_root = in_data;
  if (in_data.any_of<episodes>()) {
    p_data = p_root.get<episodes>().get_episodes();
  }
  return true;
}

comm_ass_shot::comm_ass_shot()
    : p_root(),
      p_data(),
      p_end(),
      p_shot_ab() {
  p_name     = "镜头";
  p_show_str = make_imgui_name(this, "添加",
                               "批量添加",
                               "修改",
                               "删除",
                               "镜头",
                               "ab镜头");
}

bool comm_ass_shot::render() {
  if (!p_root.all_of<shot>()) {
    if (imgui::Button(p_show_str["添加"].c_str())) {
      p_root.emplace<shot>().set_shot(p_data);
      p_root.patch<database>(database::save{});
      p_root.get<root_ref>().set_root(g_reg()->ctx<root_ref>().root_handle());
    }
  } else {
    if (imgui::Button(p_show_str["修改"].c_str())) {
      p_root.patch<shot>([&](shot& in) {
        in.set_shot(p_data);
        in.set_shot_ab(magic_enum::enum_cast<shot::shot_ab_enum>(
                           p_shot_ab)
                           .value_or(shot::shot_ab_enum::None));
      });
      p_root.patch<database>(database::save{});
    }
    imgui::SameLine();
    if (imgui::Button(p_show_str["删除"].c_str())) {
      p_root.remove<shot>();
      p_root.patch<database>(database::save{});
    }
  }
  imgui::InputInt(p_show_str["镜头"].c_str(), &p_data, 1, 9999);
  dear::Combo{p_show_str["ab镜头"].c_str(), p_shot_ab.data()} && [this]() {
    static auto shot_enum{magic_enum::enum_names<shot::shot_ab_enum>()};
    for (auto& i : shot_enum) {
      if (imgui::Selectable(i.data(), i == p_shot_ab))
        p_shot_ab = i;
    }
  };

  return true;
}
bool comm_ass_shot::set_data(const entt::handle& in_data) {
  p_root = in_data;
  if (in_data.any_of<shot>()) {
    p_data    = p_root.get<shot>().get_shot();
    p_shot_ab = p_root.get<shot>().get_shot_ab();
  }
  return true;
}

comm_assets::comm_assets()
    : p_root(),
      p_path_list() {
  p_name     = "资产";
  p_show_str = make_imgui_name(this, "添加",
                               "修改",
                               "删除",
                               "名称",
                               "删除条目",
                               "添加条目");
}
FSys::path comm_assets::get_path() {
  FSys::path k_path{};
  for (auto& k_p : p_path_list) {
    k_path /= *k_p;
  }
  return k_path;
}

bool comm_assets::render() {
  if (!p_root.all_of<assets>()) {
    if (imgui::Button(p_show_str["添加"].c_str())) {
      p_root.emplace<assets>(get_path());
      p_root.patch<database>(database::save{});
      p_root.get<root_ref>().set_root(g_reg()->ctx<root_ref>().root_handle());
    }
  } else {
    if (imgui::Button(p_show_str["修改"].c_str())) {
      p_root.patch<assets>([&](assets& in) {
        /// @todo 设置路径
        in.set_path(get_path());
      });
      p_root.patch<database>(database::save{});
    }
    imgui::SameLine();
    if (imgui::Button(p_show_str["删除"].c_str())) {
      p_root.remove<assets>();
      p_root.patch<database>(database::save{});
    }
    if (imgui::Button(p_show_str["添加条目"].c_str())) {
      p_path_list.push_back(std::make_shared<string>("none"));
    }
  }

  decltype(p_path_list) k_l{};
  for (auto& k_p : p_path_list) {
    imgui::InputText(fmt::format("##{}", fmt::ptr(k_p.get())).c_str(), k_p.get());
    imgui::SameLine();
    if (imgui::Button(p_show_str["删除条目"].c_str())) {
      k_l.push_back(k_p);
    }
  }
  for (auto& k_p : k_l) {
    boost::remove_erase_if(p_path_list, [&](auto& in) { return in == k_p; });
  }

  return true;
}
bool comm_assets::set_data(const entt::handle& in_data) {
  p_root = in_data;
  if (in_data.any_of<assets>()) {
    for (auto& k_path : p_root.get<assets>().get_path()) {
      p_path_list.push_back(std::make_shared<string>(k_path.generic_string()));
    }
  }
  return true;
}

comm_ass_season::comm_ass_season()
    : p_root(),
      p_data(),
      p_end() {
  p_name     = "季数";
  p_show_str = make_imgui_name(this, "添加",
                               "批量添加",
                               "修改",
                               "删除",
                               "季数",
                               "结束季数");
}

bool comm_ass_season::render() {
  if (!p_root.all_of<season>()) {
    if (imgui::Button(p_show_str["添加"].c_str())) {
      p_root.emplace<season>(p_data);
      p_root.patch<database>(database::save{});
      p_root.get<root_ref>().set_root(g_reg()->ctx<root_ref>().root_handle());
    }
  } else {
    if (imgui::Button(p_show_str["修改"].c_str())) {
      p_root.patch<season>([p_data = p_data](season& in) {
        in.set_season(p_data);
      });
      p_root.patch<database>(database::save{});
    }
    imgui::SameLine();
    if (imgui::Button(p_show_str["删除"].c_str())) {
      p_root.remove<season>();
      p_root.patch<database>(database::save{});
    }
  }
  imgui::InputInt(p_show_str["季数"].c_str(), &p_data, 1, 9999);

  return true;
}
bool comm_ass_season::set_data(const entt::handle& in_data) {
  p_root = in_data;
  if (in_data.any_of<season>()) {
    p_data = p_root.get<season>().get_season();
  }
  return true;
}

comm_ass_file_attr::comm_ass_file_attr()
    : p_root(),
      p_time(),
      has_file(false),
      p_time_widget(new_object<time_widget>()),
      p_comm_str(new_object<string>()) {
  p_name     = "资产文件";
  p_show_str = make_imgui_name(this, "添加",
                               "更改",
                               "删除", "注释",
                               "添加注释");
}

bool comm_ass_file_attr::render() {
  if (!p_root.all_of<assets_file>()) {
    if (imgui::Button(p_show_str["添加"].c_str())) {
      p_root.emplace<assets_file>();
      p_root.get<root_ref>().set_root(g_reg()->ctx<root_ref>().root_handle());
      p_root.patch<database>(database::save{});
    }
  } else {
    if (imgui::Button(p_show_str["删除"].c_str())) {
      p_root.remove<assets_file>();
      p_root.patch<database>(database::save{});
    }
    p_time_widget->frame_render();
    imgui::InputText(p_show_str["注释"].c_str(), p_comm_str.get());
    imgui::SameLine();
    if (imgui::Button(p_show_str["添加注释"].c_str())) {
      auto k_com = comment{};
      k_com.set_comment(*p_comm_str);
      p_root.get_or_emplace<comment_vector>().get().push_back(std::move(k_com));
      p_root.patch<database>(database::save{});
    }
  }

  return true;
}
bool comm_ass_file_attr::set_data(const entt::handle& in_data) {
  p_root = in_data;
  if (in_data.all_of<time_point_wrap>()) {
    p_time_widget->set_time(p_root);
  }
  return true;
}

comm_assets_add::comm_assets_add()
    : p_list() {
  p_name     = "资产";
  p_show_str = make_imgui_name(this, "添加条目");
}

bool comm_assets_add::render() {
  if (!g_reg()->try_ctx<root_ref>())
    return false;
  if (imgui::Button(p_show_str["添加条目"].c_str())) {
    auto k_reg = g_reg();
    auto k_h   = make_handle(g_reg()->create());
    k_h.emplace<database>();
    k_h.get_or_emplace<root_ref>().set_root(g_reg()->ctx<root_ref>().root_handle());
    p_list.set_data(k_h);
  }
  return p_list.render();
}

bool comm_assets_add::set_data(const entt::handle& in_data) {
  return p_list.set_data(in_data);
}

}  // namespace doodle
