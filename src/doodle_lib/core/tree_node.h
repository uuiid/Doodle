#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {

template <class T>
class DOODLELIB_API tree_node {
 public:
  using data_type           = T;
  using data_type_ptr       = data_type*;
  using data_type_ref       = data_type&;
  using const_data_type_ref = const data_type&;

  struct iterator_up {
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = data_type;
    using pointer           = data_type_ptr;  // or also value_type*
    using reference         = data_type_ref;  // or also value_type&

    Iterator_up(tree_node* in_ptr) : m_ptr(in_ptr) {}
    reference operator*() const { return m_ptr->data; };
    pointer operator->() const { return &(m_ptr->data); };

    iterator_up& operator++() {
      m_ptr = m_ptr->parent;
      return *this;
    }

    iterator_up operator++(int) {
      iterator_up l_tmp{*this};
      ++(*this);
      return l_tmp;
    }

    friend bool operator==(const Iterator& a, const Iterator& b) { return a.m_ptr == b.m_ptr; };
    friend bool operator!=(const Iterator& a, const Iterator& b) { return !(a == b); };

   private:
    tree_node m_ptr;
  };

  

 private:
  tree_node* parent;
  std::vector<std::shared_ptr<tree_node>> child;
  T data;
};
}  // namespace doodle
