//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <boost/signals2.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/menu.hpp>
#include <nana/gui/widgets/treebox.hpp>
namespace doodle {

namespace details {
template <class widget>
class draw_guard : public no_copy {
  widget& _w;

 public:
  explicit draw_guard(widget& in_widget) : _w(in_widget) {
    _w.auto_draw(false);
  }
  ~draw_guard() {
    _w.auto_draw(true);
  }
};
}  // namespace details

nana::listbox::oresolver& operator<<(nana::listbox::oresolver& oor, const ProjectPtr& in_prj);
nana::listbox::iresolver& operator>>(nana::listbox::iresolver& oor, ProjectPtr& in_prj);

nana::listbox::oresolver& operator<<(nana::listbox::oresolver& oor, const AssetsFilePtr& in_file);
nana::listbox::iresolver& operator>>(nana::listbox::iresolver& oor, AssetsFilePtr& in_file);

//class widget {
// protected:
//  nana::window _win;
//
// public:
//  explicit widget(nana::window in_window) : _win(in_window){};
//  virtual ~widget() = default;
//};

namespace details {
class pej_widget_base {
 protected:
  nana::menu p_menu;
  std::shared_ptr<menu_factory_base> p_factory;

 public:
  boost::signals2::signal<void(const MetadataPtr& in_, bool is_selected)> sig_selected;
  virtual nana::widget& get_widget() = 0;
};
}  // namespace details

class DOODLELIB_API project_widget : public details::pej_widget_base {
  nana::listbox p_list_box;

 public:
  explicit project_widget(nana::window in_window);

  nana::listbox& get_widget() override;
};

class DOODLELIB_API assets_widget : public details::pej_widget_base {
  nana::treebox p_tree_box;

  MetadataPtr p_root;

  std::vector<boost::signals2::scoped_connection> p_conn;
  void install_solt(const MetadataPtr& in_project_ptr);
  //  nana::treebox::item_proxy append_tree(nana::treebox::item_proxy& in_proxy, MetadataPtr& in_ptr);
  void add_nodes(const MetadataPtr& in_parent);
  void add_node(const MetadataPtr& in_node,  nana::treebox::item_proxy& in_parent);
 public:
  explicit assets_widget(nana::window in_window);

  void set_ass(const MetadataPtr& in_project_ptr);
  void clear();

  nana::treebox& get_widget() override;
};

class DOODLELIB_API assets_attr_widget : public details::pej_widget_base {
  nana::listbox p_list_box;
  MetadataPtr p_root;

  std::vector<AssetsFilePtr> p_assets;
  std::vector<boost::signals2::scoped_connection> p_conn;
  void install_sig();
  void install_sig_one(std::shared_ptr<AssetsFile>& k_file);

 public:
  explicit assets_attr_widget(nana::window in_window);
  void set_ass(const MetadataPtr& in_ptr);
  void clear();
  nana::listbox& get_widget() override;
};
}  // namespace doodle
