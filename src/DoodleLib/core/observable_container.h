//
// Created by TD on 2021/6/24.
//

#pragma once

#include <boost/signals2.hpp>
#include <iterator>
#include <string>
#include <vector>

namespace doodle {

template <class container_type = std::vector<std::string> >
class observable_container : public container_type {
 public:
//  using _container_type  = typename std::vector<std::string>;
  using _allocator_type  = typename container_type::allocator_type;
  using _size_type       = typename container_type::size_type;
  using _value_type      = typename container_type::value_type;
  using _pointer         = typename container_type::pointer;
  using _const_pointer   = typename container_type::const_pointer;
  using _reference       = typename container_type::reference;
  using _const_reference = typename container_type::const_reference;
  using _difference_type = typename container_type::difference_type;

  using _iterator               = typename container_type::iterator;
  using _const_iterator         = typename container_type::const_iterator;
  using _reverse_iterator       = typename container_type::reverse_iterator;
  using _const_reverse_iterator = typename container_type::const_reverse_iterator;

  observable_container() : container_type(){};
  explicit observable_container(const _allocator_type& _al) : container_type(_al){};
  explicit observable_container(const _size_type _count, const _allocator_type& _al = _allocator_type{})
      : container_type(_count, _al){};
  //  explicit observable_container(const )

  void clear_sig() {
    container_type::clear();
    sig_clear();
  };

  _iterator insert_sig(_const_iterator _where, const _value_type& _val) {
    auto _k_where = container_type::insert(_where, _val);
    sig_insert(_k_where);
    return _k_where;
  };

  _iterator insert_sig(_const_iterator _where, _value_type&& _val) {
    auto _k_where = container_type::insert(_where, std::move(_val));
    sig_insert(_k_where);
    return _k_where;
  };

  _iterator erase_sig(_iterator _where) {
    auto _k_where = container_type::erase(_where);
    sig_erase(_k_where);
    return _k_where;
  };

  _iterator erase_sig(_const_iterator _where) {
    auto _k_where = container_type::erase(_where);
    sig_erase(_k_where);
    return _k_where;
  }

  void push_back_sig(const _value_type& _val){
    container_type::push_back(_val);
    sig_push_back();
  };

  void push_back_sig(_value_type&& _val){
    container_type::push_back(std::move(_val));
    sig_push_back();
  };

  void resize_sig(_size_type _count){
    container_type::resize(_count);
    sig_resize(_count);
  }
  //  _iterator insert(_const_iterator _where, const _size_type _count, const _value_type& _val) {
  //    auto _k_where = container_type::insert(_where, _count, _val);
  //    sig_insert(_k_where);
  //    return _k_where;
  //  };
  //
  //  template <class _iter, std::enable_if_t<std::is_integral_v<_iter>, int> = 0>
  //  _iterator insert(_const_iterator _where, _iter _first, _iter _last){
  //      auto _k_where = container_type::insert(_where,_first,_last);
  //      sig_insert(_k_where);
  //      return _k_where;
  //  };
  boost::signals2::signal<void()> sig_clear;
  boost::signals2::signal<void(_const_iterator where)> sig_insert;
  boost::signals2::signal<void(_iterator where)> sig_erase;
  boost::signals2::signal<void()> sig_push_back;
  boost::signals2::signal<void(_const_iterator where)> sig_resize;
};
using my_str = observable_container<std::vector<std::string>>;

}  // namespace doodle
