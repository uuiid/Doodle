//
// Created by td_main on 2023/6/16.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include "doodle_app/gui/base/ref_base.h"

#include "boost/operators.hpp"

#include <string>
#include <treehh/tree.hh>
#include <utility>
namespace doodle::gui {

class assets_tree {
  struct assets_tree_node : boost::less_than_comparable<assets_tree_node> {
   public:
    std::string name;
    bool has_select{};
    entt::handle handle{};
    assets_tree_node() = default;

    explicit assets_tree_node(std::string in_name, const entt::handle& in_handle)
        : name(std::move(in_name)), handle(in_handle) {}
    bool operator<(const assets_tree_node& rhs) const;
  };

  using tree_type_t = tree<assets_tree_node>;
  tree_type_t tree_;

  gui_cache_name_id input_add_node{"名称"};
  std::string node_name{};

  void build_tree(const entt::handle& in_handle_view, const tree_type_t::iterator& in_parent);
  bool render_child(const tree_type_t::iterator& in_node);
  void popen_menu(const tree_type_t::iterator_base& in);

 public:
  assets_tree() = default;

  void init_tree();
  bool render();
};

}  // namespace doodle::gui
