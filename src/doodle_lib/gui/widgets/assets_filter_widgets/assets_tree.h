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
  tree<assets_tree_node> tree_;

  bool render_child(const tree<assets_tree_node>::iterator& in);

  void popen_menu(const tree<assets_tree_node>::iterator_base& in);

 public:
  assets_tree() = default;

  bool render();
};

}  // namespace doodle::gui
