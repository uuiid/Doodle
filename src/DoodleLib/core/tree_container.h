//
// Created by TD on 2021/7/30.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <boost/intrusive/intrusive_fwd.hpp>
#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/pack_options.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/trivial_value_traits.hpp>

namespace doodle {

using set_base_hook = boost::intrusive::set_base_hook<
    boost::intrusive::link_mode<boost::intrusive::safe_link>>;

class tree_node;
using tree_node_ptr = std::shared_ptr<tree_node>;

template <class... type_arg>
tree_node_ptr make_tree(type_arg&&... in_arg);

namespace details {
class tree_node_destroy : public std::default_delete<tree_node> {
 public:
  tree_node_destroy() = default;
  void operator()(tree_node* in_ptr);
};
}  // namespace details

/**
 * @brief 这是一个树结构
 */
class tree_node : public std::enable_shared_from_this<tree_node>,
                  public set_base_hook,
                  public details::no_copy {
  friend details::tree_node_destroy;
  tree_node();
  explicit tree_node(tree_node* in_parent, MetadataPtr in_data);
  explicit tree_node(const tree_node_ptr& in_parent, MetadataPtr in_data);

 public:
  ~tree_node();
  using tree_node_ptr = std::shared_ptr<tree_node>;

  using child_set = boost::intrusive::set<
      tree_node,
      boost::intrusive::base_hook<tree_node>,
      boost::intrusive::constant_time_size<false>>;

  template <class... type_arg>
  static tree_node_ptr make_this(type_arg&&... in_arg) {
    auto tmp = std::shared_ptr<tree_node>{
        new tree_node{std::forward<type_arg>(in_arg)...},
        details::tree_node_destroy()};
    //    if constexpr (sizeof...(in_arg) > 1) {
    //      if (tmp->has_parent())
    //        tmp->parent->insert(tmp);
    //    }
    return tmp;
  };

  [[nodiscard]] bool is_root() const;
  [[nodiscard]] bool has_parent() const;
  [[nodiscard]] tree_node_ptr get_parent() const;
  [[nodiscard]] const child_set& get_children() const;

  void insert(const tree_node_ptr& in_);
  void insert(tree_node& in_);
  void remove(const tree_node_ptr& in_);
  void remove(tree_node& in_);
  void clear();

  bool operator==(const tree_node& in_rhs) const;
  bool operator!=(const tree_node& in_rhs) const;

  bool operator<(const tree_node& in_rhs) const;
  bool operator>(const tree_node& in_rhs) const;
  bool operator<=(const tree_node& in_rhs) const;
  bool operator>=(const tree_node& in_rhs) const;

   operator MetadataPtr&();
   MetadataPtr& get();

   operator const MetadataPtr&() const;
   const MetadataPtr& get() const;

 private:
  tree_node* parent;
  MetadataPtr data;
  child_set child_item;
};

template <class... type_arg>
tree_node_ptr make_tree(type_arg&&... in_arg) {
  return tree_node::make_this(std::forward<type_arg>(in_arg)...);
}

// class tree_meta;
// class tree_meta {
//  public:
//   using tree_meta_ptr      = std::shared_ptr<tree_meta>;
//   using tree_meta_weak_ptr = std::weak_ptr<tree_meta>;
//
//  private:
//   tree_meta_weak_ptr parent;
//   std::set<tree_meta_ptr> child_item;
//   MetadataPtr data;
//
//  public:
//   tree_meta() = default;
//   explicit tree_meta(tree_meta_ptr& in_meta, MetadataPtr in_metadata);
//   //  bool has_parent();
//   //  bool is_root();
// };
}  // namespace doodle
