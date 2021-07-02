//
// Created by TD on 2021/6/28.
//

#include "project_widget.h"

#include <Gui/action/action_import.h>
#include <Gui/factory/menu_factory.h>
#include <Metadata/AssetsFile.h>
#include <Metadata/Comment.h>
#include <Metadata/Project.h>
#include <Metadata/TimeDuration.h>
#include <libWarp/nana_warp.h>
namespace doodle {

project_widget::project_widget(nana::window in_window)
    : p_list_box(in_window),
      p_menu() {
  p_list_box.append_header("名称");
  p_list_box.append_header("根目录");
  p_list_box.append_header("英文名称");

  p_list_box.clear();

  details::draw_guard<nana::listbox> k_guard{p_list_box};

  for (auto& k_i : CoreSet::getSet().p_project_vector) {
    p_list_box.at(0).append(k_i, true);
  }
  p_list_box.events().selected([](const nana::arg_listbox& in_) {
    in_.item.value<ProjectPtr>();
  });
  p_list_box.events().mouse_down(menu_assist{[this](const nana::arg_mouse& in_) {
    p_menu.clear();

    menu_factory k_factory{in_.window_handle};
    auto k_selected = p_list_box.selected();
    if (k_selected.empty())
      return;

    auto k_pair = k_selected.at(0);
    MetadataPtr k_ptr  = p_list_box.at(k_pair).value<ProjectPtr>();
    k_factory.create_prj_action(k_ptr);
    k_factory(p_menu);
    p_menu.popup(in_.window_handle, in_.pos.x, in_.pos.y);
  }});

  //  nana::API::refresh_window(p_list_box);
}
nana::listbox& project_widget::get_widget() {
  return p_list_box;
}

nana::listbox::oresolver& operator<<(nana::listbox::oresolver& oor, const ProjectPtr& prj) {
  oor << prj->showStr();
  oor << prj->getPath().generic_string();
  oor << prj->str();
  return oor;
}
nana::listbox::iresolver& operator>>(nana::listbox::iresolver& oor, ProjectPtr& prj) {
  return oor;
}
nana::listbox::oresolver& operator<<(nana::listbox::oresolver& oor, const AssetsFilePtr& in_file) {
  oor << in_file->getId()
      << in_file->getVersionStr()
      << in_file->showStr();
  auto k_com = in_file->getComment();
  if (k_com.empty())
    oor << "none";
  else
    oor << k_com.back()->getComment();

  oor << in_file->getTime()->showStr()
      << in_file->getUser();

  return oor;
}
nana::listbox::iresolver& operator>>(nana::listbox::iresolver& oor, AssetsFilePtr& in_file) {
  return oor;
}
assets_widget::assets_widget(nana::window in_window)
    : p_tree_box(in_window),
      p_menu(),
      p_root() {
  p_tree_box.events().selected([this](const nana::arg_treebox& in_) {
    auto k_ptr = in_.item.value<MetadataPtr>();
    if (k_ptr)
      sig_selected(k_ptr);
    DOODLE_LOG_INFO("选中 {}", in_.item.key())
  });
  p_tree_box.events().mouse_down(menu_assist{[this](const nana::arg_mouse& in_) {
    p_menu.clear();

    menu_factory k_factory{in_.window_handle};
    auto k_selected = p_tree_box.selected();
    if (k_selected.empty())
      return;
    MetadataPtr k_ptr = k_selected.value<MetadataPtr>();
    k_factory.create_ass_action(k_ptr);
    k_factory(p_menu);
    p_menu.popup(in_.window_handle, in_.pos.x, in_.pos.y);
  }});
  p_tree_box.events()
      .expanded([this](const nana::arg_treebox& in_) {
        auto k_proxy = in_.item;
        DOODLE_LOG_INFO("扩展 {}", k_proxy.key())
        details::draw_guard<nana::treebox> k_guard{p_tree_box};

        k_proxy.clear();
        auto k_me = k_proxy.value<MetadataPtr>();
        k_me->select_indb();

        for (auto& k_i : k_me->child_item) {
          k_proxy.append(k_i->str(), k_i->showStr(), k_i);
        }
      });
}
void assets_widget::set_ass(const MetadataPtr& in_project_ptr) {
  p_tree_box.clear();
  if (!in_project_ptr) {
    DOODLE_LOG_INFO("传入空指针")
    return;
  }
  p_root = in_project_ptr;
  in_project_ptr->select_indb();
  if (!in_project_ptr->hasChild()) return;

  details::draw_guard<nana::treebox> k_guard{p_tree_box};

  for (auto& k_i : in_project_ptr->child_item) {
    auto k_item = p_tree_box.insert(k_i->str(), k_i->showStr());
    k_item.value(k_i);
    DOODLE_LOG_INFO("树文件路径: {}", k_item.key());
    if (k_i->hasChild()) {
      k_item.append("none", "none");
    }
  }
}

nana::treebox& assets_widget::get_widget() {
  return p_tree_box;
}

assets_attr_widget::assets_attr_widget(nana::window in_window)
    : p_list_box(in_window),
      p_menu(),
      p_assets() {
  p_list_box.append_header("id");
  p_list_box.append_header("版本");
  p_list_box.append_header("名称");
  p_list_box.append_header("评论", 200);
  p_list_box.append_header("时间", 130);
  p_list_box.append_header("制作人");

  p_list_box.events().mouse_down(menu_assist{[this](const nana::arg_mouse& in_) {
    p_menu.clear();

    menu_factory k_factory{in_.window_handle};
    auto k_selected = p_list_box.selected();
    if (!k_selected.empty())
      return;

    auto k_pair = k_selected.at(0);
    MetadataPtr k_ptr  = p_list_box.at(k_pair).value<AssetsFilePtr>();
    k_factory.create_file_action(k_ptr);
    k_factory(p_menu);
    p_menu.popup(in_.window_handle, in_.pos.x, in_.pos.y);
  }});
}
void assets_attr_widget::set_ass(const MetadataPtr& in_ptr) {
  p_list_box.clear();
  if (!in_ptr) {
    DOODLE_LOG_WARN("传入空镜头")
    return;
  }
  in_ptr->select_indb();

  if (!in_ptr->hasChild()) {
    DOODLE_LOG_WARN("选中项没有子项")
    return;
  }

  details::draw_guard<nana::listbox> k_guard{p_list_box};

  for (auto& k_i : in_ptr->child_item) {
    if (!details::is_class<AssetsFile>(k_i)) {
      DOODLE_LOG_WARN("项目不是文件项")
      continue;
    }
    auto k_file = std::dynamic_pointer_cast<AssetsFile>(k_i);
    if (!k_file) {
      continue;
    }

    p_list_box.at(0).append(k_file, true);
  }
}
nana::listbox& assets_attr_widget::get_widget() {
  return p_list_box;
}
}  // namespace doodle
