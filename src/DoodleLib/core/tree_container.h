//
// Created by TD on 2021/7/30.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {
template <class node_type>
class tree_container : std::enable_shared_from_this<tree_container<node_type>> {
  std::set<std::shared_ptr<tree_container<node_type>>> child_item;
  node_type date;
  std::weak_ptr<tree_container<node_type>> p_parent;

 public:
  using tree_container_ptr       = std::shared_ptr<tree_container<node_type>>;
  using tree_container_const_ptr = std::shared_ptr<const tree_container<node_type>>;
  tree_container()
      : child_item(),
        date(),
        p_parent(){};

  [[nodiscard]] bool is_root() const {
    return p_parent.expired();
  };

  [[nodiscard]] bool has_parent() const {
    return p_parent;
  }
  [[nodiscard]] tree_container_ptr parent() const {
    return p_parent.lock();
  }

  [[nodiscard]] tree_container_const_ptr get_root() const {
    auto k_p = this->shared_from_this();
    while (!k_p->p_parent.expired()) {
      k_p = k_p->p_parent.lock();
    }
    return k_p;
  }
  [[nodiscard]] tree_container_ptr get_root() {
    auto k_p = this->shared_from_this();
    while (!k_p->p_parent.expired()) {
      k_p = k_p->p_parent.lock();
    }
    return k_p;
  }

}
}  // namespace doodle
