//
// Created by TD on 2021/9/18.
//

#include "command_meta.h"

#include <doodle_lib/Gui/widgets/assets_widget.h>
#include <doodle_lib/Gui/widgets/time_widget.h>
#include <doodle_lib/Metadata/metadata_cpp.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/core/open_file_dialog.h>
#include <doodle_lib/doodle_app.h>
#include <doodle_lib/lib_warp/imgui_warp.h>

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
    auto k_prj = new_object<project>(*p_prj_path, *p_prj_name);
    k_prj->updata_db(k_d_lib.get_metadata_factory());
    k_d_lib.p_project_vector = k_d_lib.get_metadata_factory()->getAllProject();
  }
  if (p_root) {
    imgui::SameLine();
    if (imgui::Button(p_show_str["修改"].c_str())) {
      p_root->set_name(*p_prj_name);
      p_root->set_path(*p_prj_path);
      p_root->updata_db();
    }
    imgui::SameLine();
    if (imgui::Button(p_show_str["删除"].c_str())) {
      p_root->deleteData();
      k_d_lib.p_project_vector = k_d_lib.get_metadata_factory()->getAllProject();
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
bool comm_project_add::set_child(const project_ptr& in_ptr) {
  p_root      = in_ptr;
  *p_prj_name = p_root->get_name();
  *p_prj_path = p_root->get_path().generic_string();
  return true;
}

void comm_ass_eps::add_eps(const std::vector<std::int32_t>& p_eps) {
  for (auto i : p_eps) {
    auto eps = new_object<episodes>(p_meta_var, i);
    p_meta_var->get_child().push_back(eps);
    eps->insert_into();
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
        p_root->set_episodes(p_data);
        p_root->updata_db();
      }

      if (!p_root->has_child() && !p_root->has_file()) {
        imgui::SameLine();
        if (imgui::Button(p_show_str["删除"].c_str())) {
          auto k_parent = p_root->get_parent();
          k_parent->get_child().erase(p_root);
          p_root->deleteData();
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
bool comm_ass_eps::set_child(const episodes_ptr& in_ptr) {
  p_root = in_ptr;
  p_data = p_root->get_episodes();
  return true;
}

void comm_ass_shot::add_shot(const std::vector<std::int32_t>& p_shots) {
  for (auto s : p_shots) {
    auto k_s = new_object<shot>();
    k_s->set_shot(s);
    k_s->set_shot_ab(std::string{p_shot_ab});
    p_meta_var->get_child().push_back(k_s);
    k_s->insert_into();
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
        p_root->set_shot(p_data);
        p_root->set_shot_ab(magic_enum::enum_cast<shot::shot_ab_enum>(
                                p_shot_ab)
                                .value_or(shot::shot_ab_enum::None));
        p_root->updata_db();
      }
      if (!p_root->has_child() && !p_root->has_file()) {
        imgui::SameLine();
        if (imgui::Button(p_show_str["删除"].c_str())) {
          auto k_parent = p_root->get_parent();
          k_parent->get_child().erase(p_root);
          p_root->deleteData();
          p_shot_ab = {};
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
bool comm_ass_shot::set_child(const shot_ptr& in_ptr) {
  p_root    = in_ptr;
  p_data    = p_root->get_shot();
  p_shot_ab = p_root->get_shot_ab();
  return true;
}

void comm_assets::add_ass(std::vector<string> in_Str) {
  for (auto& i : in_Str) {
    auto k_ass = new_object<assets>();
    k_ass->set_name1(i);
    p_meta_var->get_child().push_back(k_ass);
    k_ass->insert_into();
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
        p_root->set_name1(p_data);
        p_root->updata_db();
      }
      if (!p_root->has_child() && !p_root->has_file()) {
        imgui::SameLine();
        if (imgui::Button(p_show_str["删除"].c_str())) {
          auto k_parent = p_root->get_parent();
          k_parent->get_child().erase(p_root);
          p_root->deleteData();
        }
      }
    }
    imgui::InputText(p_show_str["名称"].c_str(), &p_data);
  }

  return true;
}
bool comm_assets::set_child(const assets_ptr& in_ptr) {
  p_root = in_ptr;
  p_data = p_root->get_name1();
  return true;
}

void comm_ass_season::add_season(const std::vector<std::int32_t>& in) {
  for (auto& i : in) {
    auto s = new_object<season>();
    s->set_season(i);
    p_meta_var->get_child().push_back(s);
    s->insert_into();
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
        p_root->set_season(p_data);
        p_root->updata_db();
      }

      if (!p_root->has_child() && !p_root->has_file()) {
        imgui::SameLine();
        if (imgui::Button(p_show_str["删除"].c_str())) {
          auto k_parent = p_root->get_parent();
          k_parent->get_child().erase(p_root);
          p_root->deleteData();
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
bool comm_ass_season::set_child(const season_ptr& in_ptr) {
  p_root = in_ptr;
  p_data = p_root->get_season();
  return true;
}

comm_ass_file::comm_ass_file()
    : p_root(),
      p_time(),
      p_comm(),
      has_file(false),
      p_time_widget(new_object<time_widget>()),
      p_comm_str(new_object<string>()) {
  p_name     = "资产文件";
  p_show_str = make_imgui_name(this, "添加",
                               "更改",
                               "删除", "注释",
                               "添加注释");
  p_time_widget->sig_time_change.connect([this]() {
    p_root->updata_db();
  });
}

bool comm_ass_file::render() {
  if (p_meta_var) {
    if (imgui::Button(p_show_str["添加"].c_str())) {
      auto ass = new_object<assets_file>();
      p_meta_var->get_child().push_back(ass);
      ass->insert_into();
    }
    if (p_root) {
      imgui::SameLine();
      if (imgui::Button(p_show_str["更改"].c_str())) {
        p_root->updata_db();
      }
      if (p_root->has_child()) {
        imgui::SameLine();
        if (imgui::Button(p_show_str["删除"].c_str())) {
          auto k_parent = p_root->get_parent();
          k_parent->get_child().erase(p_root);
          p_root->deleteData();
        }
      }
    }

    p_time_widget->frame_render();
    imgui::InputText(p_show_str["注释"].c_str(), p_comm_str.get());
    imgui::SameLine();
    if (imgui::Button(p_show_str["添加注释"].c_str())) {
      auto k_com = p_comm->get().emplace_back(new_object<comment>());
      k_com->set_comment(*p_comm_str);
      p_root->updata_db();
    }
  }

  return true;
}
bool comm_ass_file::set_child(const assets_file_ptr& in_ptr) {
  p_root = in_ptr;
  if (!p_root->get_comment())
    p_root->set_comment(new_object<comment_vector>());
  if (!p_root->get_path_file())
    p_root->set_path_file(new_object<assets_path_vector>());
  p_comm = p_root->get_comment();
  p_time_widget->set_time(p_root->get_time());
  return true;
}

comm_ass::comm_ass()
    : p_val() {
  // p_val = std::move(boost::hana::make_tuple(comm_ass_season{},
  //                                           comm_ass_eps{},
  //                                           comm_ass_shot{},
  //                                           comm_assets{},
  //                                           comm_ass_ue4_create_shot{}));
}
bool comm_ass::render() {
  boost::hana::for_each(p_val, [](auto& in) {
    dear::TreeNode{in.class_name().c_str()} && [&]() {
      in.render();
    };
  });
  return true;
}
bool comm_ass::set_child(const episodes_ptr& in_ptr) {
  boost::hana::for_each(p_val, [&](auto& in_) { in_.add_data(p_meta_var,in_ptr); });
  return command_base::set_child(in_ptr);
}
bool comm_ass::set_child(const shot_ptr& in_ptr) {
  boost::hana::for_each(p_val, [&](auto& in_) { in_.add_data(p_meta_var,in_ptr); });
  return command_base::set_child(in_ptr);
}
bool comm_ass::set_child(const season_ptr& in_ptr) {
  boost::hana::for_each(p_val, [&](auto& in_) { in_.add_data(p_meta_var,in_ptr); });
  return command_base::set_child(in_ptr);
}
bool comm_ass::set_child(const assets_ptr& in_ptr) {
  boost::hana::for_each(p_val, [&](auto& in_) { in_.add_data(p_meta_var,in_ptr); });
  return command_base::set_child(in_ptr);
}
bool comm_ass::set_child(const assets_file_ptr& in_ptr) {
  boost::hana::for_each(p_val, [&](auto& in_) { in_.add_data(p_meta_var,in_ptr); });
  return command_base::set_child(in_ptr);
}
bool comm_ass::set_child(const project_ptr& in_ptr) {
  boost::hana::for_each(p_val, [&](auto& in_) { in_.add_data(p_meta_var,in_ptr); });
  return command_base::set_child(in_ptr);
}
bool comm_ass::set_child(nullptr_t const& in_ptr) {
  boost::hana::for_each(p_val, [&](auto& in_) { in_.add_data(p_meta_var,in_ptr); });
  return command_base::set_child(in_ptr);
}

}  // namespace doodle
