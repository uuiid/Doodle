//
// Created by TD on 2021/9/18.
//

#include "command_meta.h"

#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/core/open_file_dialog.h>
#include <doodle_lib/doodle_app.h>
#include <doodle_lib/gui/widgets/assets_widget.h>
#include <doodle_lib/gui/widgets/time_widget.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata_cpp.h>

#include <boost/algorithm/algorithm.hpp>
#include <boost/range.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/irange.hpp>
namespace doodle {

comm_project_add::comm_project_add()
    : p_prj_name(new_object<string>()),
      p_prj_name_short(new_object<string>()),
      p_prj_path(new_object<string>()),
      p_root() {
  p_name     = "项目";
  p_show_str = make_imgui_name(this, "删除", "添加",
                               "修改", "名称",
                               "路径", "选择");
}

bool comm_project_add::render() {
  auto& k_d_lib = doodle_lib::Get();

  if (imgui::Button(p_show_str["添加"].c_str())) {
    auto k_en = make_handle(reg->create());
    k_en.emplace<project>(*p_prj_path, *p_prj_name);
    k_en.emplace<need_save>();
    k_d_lib.p_project_vector.push_back(k_en);
  }
  if (p_root) {
    imgui::SameLine();
    if (imgui::Button(p_show_str["修改"].c_str())) {
      p_root.get<project>().set_name(*p_prj_name);
      p_root.get<project>().set_path(*p_prj_path);
      p_root.emplace<need_save>();
    }
    imgui::SameLine();
    if (imgui::Button(p_show_str["删除"].c_str())) {
      p_root.emplace<need_delete>();
    }
  }

  imgui::InputText(p_show_str["名称"].c_str(), p_prj_name.get());
  imgui::InputText(p_show_str["路径"].c_str(), p_prj_path.get());
  imgui::SameLine();
  if (imgui::Button(p_show_str["选择"].c_str())) {
    open_file_dialog{"open_select_path",
                     "选择路径",
                     nullptr,
                     ".",
                     "",
                     1}
        .show(
            [this](const std::vector<FSys::path>& in) {
              *p_prj_path = in.front().generic_string();
            });
  }

  return true;
}
bool comm_project_add::set_data(const entt::handle& in_data) {
  if (in_data.any_of<project>()) {
    p_root = in_data;
    if (p_root) {
      *p_prj_name = p_root.get<project>().get_name();
      *p_prj_path = p_root.get<project>().get_path().generic_string();
    }
  } else {
    p_root = entt::null;
  }
  return true;
}

void comm_ass_eps::add_eps(const std::vector<std::int32_t>& p_eps) {
  for (auto i : p_eps) {
    auto eps = make_handle(reg->create());
    eps.emplace<episodes>(i);
    eps.emplace<need_save>();
    p_meta_var.get<tree_relationship>().get_child().push_back(eps);
    p_meta_var.emplace<need_save>();
  }
}

comm_ass_eps::comm_ass_eps()
    : p_root(),
      p_data(0),
      use_batch(new_object<bool>(false)) {
  p_name     = "集数";
  p_show_str = make_imgui_name(this, "添加",
                               "批量添加",
                               "修改",
                               "删除",
                               "集数",
                               "结束集数");
}

bool comm_ass_eps::render() {
  ImGui::BulletText("%s", p_name.c_str());
  if (p_meta_var) {
    if (imgui::Button(p_show_str["添加"].c_str())) {
      if (!(*use_batch)) {
        p_end = p_data + 1;
      }
      add_eps(range(p_data, p_end));
    }
    imgui::SameLine();
    imgui::Checkbox(p_show_str["批量添加"].c_str(), use_batch.get());

    if (p_root) {
      imgui::SameLine();
      if (imgui::Button(p_show_str["修改"].c_str())) {
        p_root.patch<episodes>([&](auto& eps) { eps.set_episodes(p_data) });
        p_root.emplace<need_save>();
      }

      if (!p_root.get<database>().has_child() && !p_root.get<database>().has_file()) {
        imgui::SameLine();
        if (imgui::Button(p_show_str["删除"].c_str())) {
          p_root.emplace<need_delete>();
        }
      }
    }
    imgui::InputInt(p_show_str["集数"].c_str(), &p_data, 1, 9999);
    if (*use_batch)
      imgui::InputInt(p_show_str["结束集数"].c_str(), &p_end, 1, 9999);
    if (p_end < p_data)
      p_end = p_data + 1;
  }
  return true;
}
bool comm_ass_eps::set_data(const entt::handle& in_data) {
  if (in_data.any_of<episodes>()) {
    p_root = in_data;
    p_data = p_root.get<episodes>().get_episodes();
  } else {
    p_root = entt::null;
  }
  return true;
}

void comm_ass_shot::add_shot(const std::vector<std::int32_t>& p_shots) {
  for (auto s : p_shots) {
    auto k_h = make_handle(reg->create());
    k_h.emplace<shot>(s);
    k_h.get<shot>().set_shot_ab(std::string{p_shot_ab});
    k_h.emplace<need_save>();
    p_meta_var.get<tree_relationship>().get_child().push_back(k_h);
    p_meta_var.emplace<need_save>();
  }
}

comm_ass_shot::comm_ass_shot()
    : p_root(),
      p_data(),
      p_end(),
      use_batch(new_object<bool>(false)),
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
  if (p_meta_var) {
    if (imgui::Button(p_show_str["添加"].c_str())) {
      if (!(*use_batch)) {
        p_end = p_data + 1;
      }
      add_shot(range(p_data, p_end));
    }
    imgui::SameLine();
    imgui::Checkbox(p_show_str["批量添加"].c_str(), use_batch.get());
    if (p_root) {
      imgui::SameLine();
      if (imgui::Button(p_show_str["修改"].c_str())) {
        p_root.patch<shot>([&](shot& in) {
          in.set_shot(p_data);
          in.set_shot_ab(magic_enum::enum_cast<shot::shot_ab_enum>(
                             p_shot_ab)
                             .value_or(shot::shot_ab_enum::None));
        });
        p_root.emplace<need_save>();
      }
      if (!p_root.get<database>().has_child() && !p_root.get<database>().has_file()) {
        imgui::SameLine();
        if (imgui::Button(p_show_str["删除"].c_str())) {
          p_root.emplace<need_delete>();
        }
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
    if (*use_batch) {
      imgui::InputInt("镜头结束", &p_end, 1, 9999);
    }
  }

  return true;
}
bool comm_ass_shot::set_data(const entt::handle& in_data) {
  if (in_data.any_of<shot>()) {
    p_root    = in_data;
    p_data    = p_root.get<shot>().get_shot();
    p_shot_ab = p_root.get<shot>().get_shot_ab();
  } else {
    p_root = entt::null;
  }
  return true;
}

void comm_assets::add_ass(std::vector<string> in_Str) {
  for (auto& i : in_Str) {
    auto k_h = make_handle(reg->create());
    k_h.emplace<assets>(i);
    p_meta_var.patch<tree_relationship>([&](tree_relationship& in) {
      in.get_child().push_back(k_h);
    });
    k_h.emplace<need_save>();
    p_meta_var.emplace<need_save>();
  }
}

comm_assets::comm_assets()
    : p_root() {
  p_name     = "资产";
  p_show_str = make_imgui_name(this, "添加",
                               "修改",
                               "删除",
                               "名称");
}

bool comm_assets::render() {
  if (p_meta_var) {
    if (imgui::Button(p_show_str["添加"].c_str())) {
      add_ass({p_data});
    }
    if (p_root) {
      imgui::SameLine();
      if (imgui::Button(p_show_str["修改"].c_str())) {
        p_root.patch<assets>([&](assets& in) {
          in.set_name1(p_data);
        });
        p_root.emplace<need_save>();
      }
      if (!p_root.get<database>().has_child() && !p_root.get<database>().has_file()) {
        imgui::SameLine();
        if (imgui::Button(p_show_str["删除"].c_str())) {
          p_root.emplace<need_delete>();
        }
      }
    }
    imgui::InputText(p_show_str["名称"].c_str(), &p_data);
  }

  return true;
}
bool comm_assets::set_data(const entt::handle& in_data) {
  if (in_data.any_of<assets>()) {
    p_root = in_data;
    p_data = p_root.get<assets>().get_name1();
  } else {
    p_root = entt::null;
  }
  return true;
}

void comm_ass_season::add_season(const std::vector<std::int32_t>& in) {
  for (auto& i : in) {
    auto k_h = make_handle(reg->create());
    k_h.emplace<season>();
    p_meta_var.patch<tree_relationship>([k_h](tree_relationship& in_) {
      in_.get_child().push_back(k_h);
    });

    k_h.emplace<need_save>();
    p_meta_var.emplace<need_save>();
  }
}

comm_ass_season::comm_ass_season()
    : p_root(),
      p_data(),
      p_end(),
      use_batch(new_object<bool>(false)) {
  p_name     = "季数";
  p_show_str = make_imgui_name(this, "添加",
                               "批量添加",
                               "修改",
                               "删除",
                               "季数",
                               "结束季数");
}

bool comm_ass_season::render() {
  if (p_meta_var) {
    if (imgui::Button(p_show_str["添加"].c_str())) {
      if (!(*use_batch)) {
        p_end = p_data + 1;
      }
      add_season(range(p_data, p_end));
    }
    imgui::SameLine();
    imgui::Checkbox(p_show_str["批量添加"].c_str(), use_batch.get());

    if (p_root) {
      imgui::SameLine();
      if (imgui::Button(p_show_str["修改"].c_str())) {
        p_root.patch<season>([p_data = p_data](season& in) {
          in.set_season(p_data);
        });
        p_root.emplace<need_save>();
      }

      if (!p_root.get<database>().has_child() && !p_root.get<database>().has_file()) {
        imgui::SameLine();
        if (imgui::Button(p_show_str["删除"].c_str())) {
          p_root.emplace<need_delete>();
        }
      }
    }
    imgui::InputInt(p_show_str["季数"].c_str(), &p_data, 1, 9999);
    if (*use_batch)
      imgui::InputInt(p_show_str["结束季数"].c_str(), &p_end, 1, 9999);
    if (p_end < p_data)
      p_end = p_data + 1;
  }

  return true;
}
bool comm_ass_season::set_data(const entt::handle& in_data) {
  if (in_data.any_of<season>()) {
    p_root = in_data;
    p_data = p_root.get<season>().get_season();
  } else {
    p_root = entt::null;
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
  if (p_meta_var) {
    if (imgui::Button(p_show_str["添加"].c_str())) {
      auto k_h = make_handle(reg->create());
      k_h.emplace<assets_file>();
      p_meta_var.patch<tree_relationship>([k_h](tree_relationship& in) {
        in.get_child().push_back(k_h);
      });
      k_h.emplace<need_save>();
    }
    if (p_root) {
      imgui::SameLine();
      if (!p_root.get<database>().has_child()) {
        imgui::SameLine();
        if (imgui::Button(p_show_str["删除"].c_str())) {
          p_root.emplace<need_delete>();
        }
      }
    }

    p_time_widget->frame_render();
    imgui::InputText(p_show_str["注释"].c_str(), p_comm_str.get());
    imgui::SameLine();
    if (imgui::Button(p_show_str["添加注释"].c_str())) {
      auto k_com = comment{};
      k_com.set_comment(*p_comm_str);
      p_root.get_or_emplace<comment_vector>().get().push_back(std::move(k_com));
      p_root.emplace_or_replace<need_save>();
    }
  }

  return true;
}
bool comm_ass_file_attr::set_data(const entt::handle& in_data) {
  if (in_data.all_of<assets_file,time_point_wrap>()) {
    p_root = in_data;
    p_time_widget->set_time(p_root);
  } else {
    p_root = entt::null;
  }
  return true;
}

}  // namespace doodle
