//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <boost/signals2.hpp>
#include <nana/gui/widgets/listbox.hpp>
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

nana::listbox::oresolver& operator<<(nana::listbox::oresolver& oor, const AssetsFilePtr & in_file);
nana::listbox::iresolver& operator>>(nana::listbox::iresolver& oor, AssetsFilePtr & in_file);
class DOODLELIB_API project_widget {
  nana::listbox p_list_box;

 public:
  explicit project_widget(nana::window in_window);

  nana::listbox& get_widget();
};

class DOODLELIB_API assets_widget {
  nana::treebox p_tree_box;

  MetadataPtr p_root;

 public:
  explicit assets_widget(nana::window in_window);

  void set_ass(const MetadataPtr& in_project_ptr);
  boost::signals2::signal<void(const MetadataPtr& in_)> sig_selected;
  nana::treebox& get_widget();
};

class DOODLELIB_API assets_attr_widget {
  nana::listbox p_list_box;

  std::vector<AssetsFilePtr> p_assets;

 public:
  explicit assets_attr_widget(nana::window in_window);
  void set_ass(const MetadataPtr& in_ptr);
  nana::listbox& get_widget();
};
}  // namespace doodle
