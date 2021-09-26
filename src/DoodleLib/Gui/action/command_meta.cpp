//
// Created by TD on 2021/9/18.
//

#include "command_meta.h"

#include <DoodleLib/Metadata/Metadata_cpp.h>
#include <DoodleLib/core/DoodleLib.h>
#include <DoodleLib/core/open_file_dialog.h>
#include <DoodleLib/libWarp/imgui_warp.h>

namespace doodle {
comm_project_add::comm_project_add()
    : p_prj_name(new_object<string>()),
      p_prj_name_short(new_object<string>()),
      p_prj_path(new_object<string>()),
      p_root() {
  p_name = "项目";
}

bool comm_project_add::render() {
  auto& k_d_lib = DoodleLib::Get();
  if (imgui::Button("添加")) {
    auto k_prj = new_object<Project>(*p_prj_path, *p_prj_name);
    k_prj->updata_db(k_d_lib.get_metadata_factory());
    k_d_lib.p_project_vector = k_d_lib.get_metadata_factory()->getAllProject();
  }
  if (p_root) {
    imgui::SameLine();
    if (imgui::Button("修改")) {
      p_root->setName(*p_prj_name);
      p_root->setPath(*p_prj_path);
      p_root->updata_db();
    }
    imgui::SameLine();
    if (imgui::Button("删除")) {
      p_root->deleteData();
      k_d_lib.p_project_vector = k_d_lib.get_metadata_factory()->getAllProject();
    }
  }

  imgui::InputText("名称", p_prj_name.get());
  imgui::InputText("路径", p_prj_path.get());
  imgui::SameLine();
  if (imgui::Button("选择")) {
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

bool comm_project_add::add_data(const MetadataPtr& in_parent, const MetadataPtr& in) {
  p_root = std::dynamic_pointer_cast<Project>(in);
  if (p_root) {
    *p_prj_name = p_root->getName();
    *p_prj_path = p_root->getPath().generic_string();
  }
  return p_root != nullptr;
}

void comm_ass_eps::add_eps(std::int32_t in_begin, std::int32_t in_end) {
  for (auto i = in_begin; i < in_end; ++i) {
    auto eps = new_object<Episodes>(p_parent, i);
    p_parent->add_child(eps);
    eps->insert_into();
  }
}

comm_ass_eps::comm_ass_eps()
    : p_parent(),
      p_root(),
      p_data(0),
      use_batch(new_object<bool>(false)) {
}

bool comm_ass_eps::render() {
  if (p_parent) {
    if (imgui::Button("添加")) {
      imgui::Checkbox("批量添加", use_batch.get());
      if (!use_batch) {
        p_end = p_data + 1;
      }
      add_eps(p_data, p_end);
    }
    if (p_root) {
      if (imgui::Button("修改")) {
        p_root->setEpisodes(p_data);
        p_root->updata_db();
      }
      if (!p_root->hasChild()) {
        if (imgui::Button("删除")) {
          p_root->deleteData();
        }
      }
    }
    imgui::InputInt("集数", &p_data, 1, 9999);
    if (*use_batch)
      imgui::InputInt("结束集数", &p_end, 1, 9999);
    if (p_end < p_data)
      p_end = p_data + 1;
  }
  return true;
}

bool comm_ass_eps::add_data(const MetadataPtr& in_parent, const MetadataPtr& in) {
  p_parent = in_parent;
  p_root   = std::dynamic_pointer_cast<Episodes>(in);
  if (p_root) {
    p_data = p_root->getEpisodes();
  }
  return p_root != nullptr;
}

comm_ass_shot::comm_ass_shot()
    : p_parent(),
      p_root() {
}

bool comm_ass_shot::render() {
  return true;
}

bool comm_ass_shot::add_data(const MetadataPtr& in_parent, const MetadataPtr& in) {
  p_parent = in_parent;
  p_root   = std::dynamic_pointer_cast<Shot>(in);
  if (p_root) {
    p_data    = p_root->getShot();
    p_shot_ab = magic_enum::enum_integer(p_root->getShotAb_enum());
  }
  return p_root != nullptr;
}

comm_assets::comm_assets()
    : p_parent(),
      p_root() {
}

bool comm_assets::render() {
  return true;
}

bool comm_assets::add_data(const MetadataPtr& in_parent, const MetadataPtr& in) {
  p_parent = in_parent;
  p_root   = std::dynamic_pointer_cast<Assets>(in);
  if (p_root) {
    p_data = p_root->getName1();
  }
  return p_root != nullptr;
}

comm_ass_season::comm_ass_season()
    : p_parent(),
      p_root() {
}

bool comm_ass_season::render() {
  return true;
}

bool comm_ass_season::add_data(const MetadataPtr& in_parent, const MetadataPtr& in) {
  p_parent = in_parent;
  p_root   = std::dynamic_pointer_cast<season>(in);
  if (p_root) {
    p_data = p_root->get_season();
  }
  return p_root != nullptr;
}

comm_ass_file::comm_ass_file()
    : p_parent(),
      p_root() {
}

bool comm_ass_file::render() {
  return true;
}

bool comm_ass_file::add_data(const MetadataPtr& in_parent, const MetadataPtr& in) {
  p_parent = in_parent;
  p_root   = std::dynamic_pointer_cast<AssetsFile>(in);
  if (p_root) {
    p_time = p_root->getTime()->getLocalTime();
    p_comm = p_root->getComment();
  }
  return p_root != nullptr;
}

comm_ass::comm_ass()
    : p_val() {
}
bool comm_ass::render() {
  boost::hana::for_each(p_val, [](auto& in) { in.render(); });
  return true;
}
bool comm_ass::add_data(const MetadataPtr& in_parent, const MetadataPtr& in) {
  boost::hana::for_each(p_val, [&](auto& in_) { in_.add_data(in_parent, in); });
  return true;
}
}  // namespace doodle
