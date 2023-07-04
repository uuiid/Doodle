//
// Created by td_main on 2023/6/16.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include "doodle_app/gui/base/ref_base.h"

#include "boost/operators.hpp"

#include "entt/entity/fwd.hpp"
#include <string>
#include <treehh/tree.hh>
#include <utility>
#include <vector>
namespace doodle::gui {

class assets_tree {
  struct assets_tree_node : boost::less_than_comparable<assets_tree_node> {
   public:
    gui_cache_name_id name;
    bool has_select{};
    entt::handle handle{};
    assets_tree_node() = default;

    explicit assets_tree_node(std::string in_name, const entt::handle& in_handle)
        : name(std::move(in_name)), handle(in_handle) {}
    bool operator<(const assets_tree_node& rhs) const;
  };

  using tree_type_t = tree<assets_tree_node>;
  tree_type_t tree_;

  struct {
    gui_cache_name_id input{"名称"};
    gui_cache_name_id node{"添加"};
    gui_cache_name_id node_child{"添加子集"};
    std::string node_name{};
  } input_data{};
  struct {
    gui_cache_name_id input{"名称"};
    gui_cache_name_id rename{"重命名"};
    std::string node_name{};
  } rename_data{};

  gui_cache_name_id delete_node{"删除"};

  bool edit_data{};

  entt::handle current_select_handle;
  std::vector<entt::handle> filter_list_handles{};

  void build_tree(const entt::handle& in_handle_view, const tree_type_t::iterator& in_parent);
  bool render_child(const tree_type_t::iterator& in_node);
  void popen_menu(const tree_type_t::sibling_iterator& in);

  void delete_node_(const tree_type_t::iterator_base& in_node);

  void move_node_(const tree_type_t::iterator& in_node, const tree_type_t::iterator& in_parent);

  void filter_list();

 public:
  assets_tree() = default;

  void init_tree();
  bool render();

  inline const std::vector<entt::handle>& get_filter_list_handles() const { return filter_list_handles; };
};

}  // namespace doodle::gui
