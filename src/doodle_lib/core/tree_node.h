#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {

namespace details {
template <typename It>
class iterable_adaptor final {
 public:
  /**
   * @brief 获取迭代器返回值
   *
   */
  using value_type     = typename std::iterator_traits<It>::value_type;
  /**
   * @brief 迭代器别名
   *
   */
  using iterator       = It;
  /**
   * @brief 常量迭代器
   *
   */
  using const_iterator = const iterator;

  iterable_adaptor()   = default;
  iterable_adaptor(iterator from, iterator to)
      : first{from},
        last{to} {}

  [[nodiscard]] const_iterator begin() const ENTT_NOEXCEPT {
    return first;
  }

  [[nodiscard]] const_iterator end() const ENTT_NOEXCEPT {
    return last;
  }

  [[nodiscard]] const_iterator cbegin() const ENTT_NOEXCEPT {
    return begin();
  }

  [[nodiscard]] const_iterator cend() const ENTT_NOEXCEPT {
    return end();
  }

 private:
  iterator first;
  iterator last;
}
}  // namespace details

template <class T>
class DOODLELIB_API tree_node {
 public:
  using data_type           = T;
  using data_type_ptr       = data_type*;
  using data_type_ref       = data_type&;
  using const_data_type_ref = const data_type&;

  tree_node* parent;
  std::vector<std::shared_ptr<tree_node>> child;
  T data;

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


  tree_node(T in_data)
      : parent(),
        child(),
        data(std::move(in_data)) {}

  tree_node() : tree_node(T{}){};

  bool is_root() {
    return parent == nullptr;
  }

  details::iterable_adaptor<iterator_up> each_up() {
    return {this, nullptr};
  }

  details::iterable_adaptor<decltype(child)::iterator> each_child() {
    return {child.begin(), child.end()};
  }

  details::iterable_adaptor<decltype(child)::iterator> each_current_layer() {
    if (is_root())
      return {};
    else
      return {parent->child.begin(), parent->child.end()};
  }


  constexpr operator const T&() const {
    return data;
  }
  constexpr operator T&() {
    return data;
  }

 private:
};
}  // namespace doodle
