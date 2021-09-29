//
// Created by TD on 2021/9/18.
//

#include "command_meta.h"

#include <DoodleLib/Gui/widgets/assets_widget.h>
#include <DoodleLib/Gui/widgets/time_widget.h>
#include <DoodleLib/Metadata/metadata_cpp.h>
#include <DoodleLib/core/doodle_lib.h>
#include <DoodleLib/core/open_file_dialog.h>
#include <DoodleLib/doodle_app.h>
#include <DoodleLib/libWarp/imgui_warp.h>

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

bool comm_project_add::add_data(const metadata_ptr& in_parent, const metadata_ptr& in) {
  p_root = std::dynamic_pointer_cast<project>(in);
  if (p_root) {
    *p_prj_name = p_root->get_name();
    *p_prj_path = p_root->get_path().generic_string();
  }
  return p_root != nullptr;
}

void comm_ass_eps::add_eps(const std::vector<std::int32_t>& p_eps) {
  for (auto i : p_eps) {
    auto eps = new_object<episodes>(p_parent, i);
    p_parent->child_item.push_back_sig(eps);
    eps->insert_into();
  }
}

comm_ass_eps::comm_ass_eps()
    : p_parent(),
      p_root(),
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
  if (p_parent) {
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

bool comm_ass_eps::add_data(const metadata_ptr& in_parent, const metadata_ptr& in) {
  p_parent = in_parent;
  p_root   = std::dynamic_pointer_cast<episodes>(in);
  if (p_root) {
    p_data = p_root->get_episodes();
  }
  return p_root != nullptr;
}

void comm_ass_shot::add_shot(const std::vector<std::int32_t>& p_shots) {
  for (auto s : p_shots) {
    auto k_s = new_object<shot>();
    k_s->set_shot(s);
    p_parent->child_item.push_back_sig(k_s);
  }
}

comm_ass_shot::comm_ass_shot()
    : p_parent(),
      p_root(),
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
  if (p_parent) {
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
        if (imgui::Button(p_show_str["删除"].c_str()))
          p_root->deleteData();
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

bool comm_ass_shot::add_data(const metadata_ptr& in_parent, const metadata_ptr& in) {
  p_parent = in_parent;
  p_root   = std::dynamic_pointer_cast<shot>(in);
  if (p_root) {
    p_data    = p_root->get_shot();
    p_shot_ab = p_root->get_shot_ab();
  }
  return p_root != nullptr;
}

void comm_assets::add_ass(std::vector<string> in_Str) {
  for (auto& i : in_Str) {
    auto k_ass = new_object<assets>();
    k_ass->set_name1(i);
    p_parent->child_item.push_back_sig(k_ass);
  }
}

comm_assets::comm_assets()
    : p_parent(),
      p_root() {
  p_name     = "资产";
  p_show_str = make_imgui_name(this, "添加",
                               "修改",
                               "删除",
                               "名称");
}

bool comm_assets::render() {
  if (p_parent) {
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
          p_root->deleteData();
        }
      }
    }
    imgui::InputText(p_show_str["名称"].c_str(), &p_data);
  }

  return true;
}

bool comm_assets::add_data(const metadata_ptr& in_parent, const metadata_ptr& in) {
  p_parent = in_parent;
  p_root   = std::dynamic_pointer_cast<assets>(in);
  if (p_root) {
    p_data = p_root->get_name1();
  }
  return p_root != nullptr;
}

void comm_ass_season::add_season(const std::vector<std::int32_t>& in) {
  for (auto& i : in) {
    auto s = new_object<season>();
    s->set_season(i);
    p_parent->child_item.push_back_sig(s);
  }
}

comm_ass_season::comm_ass_season()
    : p_parent(),
      p_root(),
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
  if (p_parent) {
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

bool comm_ass_season::add_data(const metadata_ptr& in_parent, const metadata_ptr& in) {
  p_parent = in_parent;
  p_root   = std::dynamic_pointer_cast<season>(in);
  if (p_root) {
    p_data = p_root->get_season();
  }
  return p_root != nullptr;
}

comm_ass_file::comm_ass_file()
    : p_parent(),
      p_root(),
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
}

bool comm_ass_file::render() {
  if (p_parent) {
    if (imgui::Button(p_show_str["添加"].c_str())) {
      auto ass = new_object<assets_file>();
      p_parent->child_item.push_back(ass);
    }
    if (p_root) {
      imgui::SameLine();
      if (imgui::Button(p_show_str["更改"].c_str())) {
        p_root->updata_db();
      }
      if (p_root->has_child()) {
        imgui::SameLine();
        if (imgui::Button(p_show_str["删除"].c_str())) {
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
    }
  }

  return true;
}

bool comm_ass_file::add_data(const metadata_ptr& in_parent, const metadata_ptr& in) {
  p_parent = in_parent;
  p_root   = std::dynamic_pointer_cast<assets_file>(in);
  if (p_root) {
    p_time = p_root->get_time()->get_local_time();
    p_comm = p_root->get_comment();
    p_time_widget->set_time(p_root->get_time());
  }
  return p_root != nullptr;
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
bool comm_ass::add_data(const metadata_ptr& in_parent, const metadata_ptr& in) {
  boost::hana::for_each(p_val, [&](auto& in_) { in_.add_data(in_parent, in); });
  return true;
}
}  // namespace doodle
