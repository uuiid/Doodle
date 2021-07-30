//
// Created by TD on 2021/7/30.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {
template <class node_type>
class tree_container : public std::enable_shared_from_this<tree_container<node_type>> {
 public:
  using tree_container_ptr       = std::shared_ptr<tree_container<node_type>>;
  using tree_container_weak_ptr  = std::weak_ptr<tree_container<node_type>>;
  using tree_container_const_ptr = std::shared_ptr<const tree_container<node_type>>;
  class iterator;
  friend iterator;

 private:
  using tree_container_set = std::set<tree_container_ptr>;
  tree_container_set child_item;
  node_type date;
  tree_container_weak_ptr p_parent;

 public:
  class iterator {
    std::deque<tree_container_ptr> p_list;

   public:
    iterator()
        : p_list(){};

    explicit iterator(tree_container_ptr in_ptr)
        : p_list() {
      if (in_ptr) {
        p_list.insert(in_ptr);
      }
    };

    iterator operator++() {
      if (p_list.empty())
        *this;
      auto k_ptr                = p_list.front();
      tree_container_set& k_set = k_ptr->child_item;
      std::copy(k_set.begin(), k_set.end(), std::back_inserter(p_list));
      p_list.pop_front();
      return *this;
    };

    iterator operator++(int) {
      ++(*this);
      iterator r{*this};
      return r;
    }

    bool operator==(const iterator& r) const { return p_list == r.p_list; };
    bool operator!=(const iterator& r) const { return p_list != r.p_list; };
  };

  tree_container()
      : std::enable_shared_from_this<tree_container<node_type>>(),
        child_item(),
        date(),
        p_parent(){};

  [[nodiscard]] bool is_root() const {
    return p_parent.expired();
  };

  [[nodiscard]] bool has_parent() const {
    return !p_parent.expired();
  };
  [[nodiscard]] tree_container_ptr parent() const {
    return p_parent.lock();
  };

  [[nodiscard]] tree_container_const_ptr get_root() const {
    auto k_p = this->shared_from_this();
    while (!k_p->p_parent.expired()) {
      k_p = k_p->p_parent.lock();
    }
    return k_p;
  };

  [[nodiscard]] tree_container_ptr get_root() {
    auto k_p = this->shared_from_this();
    while (!k_p->p_parent.expired()) {
      k_p = k_p->p_parent.lock();
    }
    return k_p;
  };
};
}  // namespace doodle
