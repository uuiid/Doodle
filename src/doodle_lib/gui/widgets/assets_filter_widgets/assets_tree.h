//
// Created by td_main on 2023/6/16.
//

#pragma once
#include "boost/operators.hpp"

#include <string>
#include <treehh/tree.hh>
namespace doodle::gui {

class assets_tree {
  struct assets_tree_node : boost::less_than_comparable<assets_tree_node> {
    std::string name;
    bool has_select{};

    bool operator<(const assets_tree_node& rhs) const;
  };

  using tree_type_t = tree<assets_tree_node>;
  tree_type_t tree_;

  bool render_child(const tree_type_t::iterator& in_node);

  void popen_menu(const tree_type_t::iterator_base& in);

 public:
  assets_tree() = default;

  bool render();
};

}  // namespace doodle::gui
