//
// Created by TD on 2021/9/29.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Exception/exception.h>
namespace doodle {

template <class value_type>
class DOODLELIB_API child_adapter {
 public:
  using value_ptr              = std::shared_ptr<value_type>;
  using value_list             = std::vector<value_ptr>;

  using iterator               = typename value_list ::iterator;
  using const_iterator         = typename value_list ::const_iterator;
  using reverse_iterator       = typename value_list ::reverse_iterator;
  using const_reverse_iterator = typename value_list ::const_reverse_iterator;

  value_list& _list;
  value_ptr _self;

  explicit child_adapter(value_list& in_value_list, value_ptr in_self)
      : _list(in_value_list),
        _self(std::move(in_self)){};

  void push_back(const value_ptr& in) {
    _list.push_back(in);
    _self->end_push_back(in);
  };
  iterator erase(const value_ptr& in) {
    auto it = std::find(_list.begin(), _list.end(), in);
    if (it == _list.end())
      throw error_iterator{"错误的迭代器"};
    auto k_r = _list.erase(it);
    _self->end_erase(in);
    return k_r;
  };

  void clear() {
    _list.clear();
    _self->end_clear();
  };

  [[nodiscard]] bool empty() const noexcept {
    return _list.empty();
  };
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
template <class value_type>
using child_adapter_ptr = std::shared_ptr<child_adapter<value_type>>;

}  // namespace doodle
