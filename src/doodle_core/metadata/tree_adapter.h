//
// Created by TD on 2021/9/29.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>
namespace doodle {

// template <class container_type, class self_type>
// auto make_vector_adapter(container_type& vector, self_type& self) {
//   return vector_adapter<container_type, self_type>{vector, self};
// };

template <class container_type, class self_type>
class DOODLE_CORE_EXPORT vector_adapter {
 public:
  using value_type             = typename container_type::value_type;
  using value_list             = container_type;

  using self_ref               = self_type&;
  using value_list_ref         = value_list&;

  using iterator               = typename value_list ::iterator;
  using const_iterator         = typename value_list ::const_iterator;
  using reverse_iterator       = typename value_list ::reverse_iterator;
  using const_reverse_iterator = typename value_list ::const_reverse_iterator;

  value_list_ref _list;
  self_ref _self;

  explicit vector_adapter(value_list_ref in_value_list, self_ref in_self)
      : _list(in_value_list),
        _self(in_self){};

  void push_back(const value_type& in) {
    _list.push_back(in);
    _self.end_push_back(in);
  };
  void push_back(value_type&& in) {
    _self.end_push_back(in);
    _list.push_back(std::move(in));
  };

  auto emplace_back(value_type&& in_ptr) {
    auto k_r = _list.emplace_back(std::forward<value_type>(in_ptr));
    _self.end_push_back(k_r);
    return k_r;
  };

  iterator erase(const value_type& in) {
    auto it = std::find(_list.begin(), _list.end(), in);
    DOODLE_CHICK(it != _list.end(),error_iterator{"错误的迭代器"});

    auto k_r = _list.erase(it);
    _self.end_erase(in);
    return k_r;
  };

  void clear() {
    _list.clear();
    _self.end_clear();
  };

  [[nodiscard]] bool empty() const noexcept {
    return _list.empty();
  };
  [[nodiscard]] decltype(_list.front()) front() const noexcept {
    return _list.front();
  }
  [[nodiscard]] decltype(_list.front()) front() noexcept {
    return _list.front();
  }
  [[nodiscard]] iterator begin() noexcept { return _list.begin(); };
  [[nodiscard]] const_iterator begin() const noexcept { return _list.begin(); };
  [[nodiscard]] iterator end() noexcept { return _list.end(); };
  [[nodiscard]] const_iterator end() const noexcept { return _list.end(); };
  [[nodiscard]] reverse_iterator rbegin() noexcept { return _list.rbegin(); };
  [[nodiscard]] const_reverse_iterator rbegin() const noexcept { return _list.rbegin(); };
  [[nodiscard]] reverse_iterator rend() noexcept { return _list.rend(); };
  [[nodiscard]] const_reverse_iterator rend() const noexcept { return _list.rend(); };
  [[nodiscard]] const_reverse_iterator crbegin() const noexcept { return _list.crbegin(); };
  [[nodiscard]] const_reverse_iterator crend() const noexcept { return _list.crend(); };
};

}  // namespace doodle
