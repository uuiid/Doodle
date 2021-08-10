//
// Created by TD on 2021/6/24.
//

#pragma once

#include <boost/signals2.hpp>
#include <iterator>
#include <string>
#include <vector>

namespace doodle {

namespace details {

/**
 * @brief 纯虚的预处理类
 * @tparam container_type 容器类型
 */
template <class container_type>
class abs_pretreatment {
 public:
  using _value_type  = typename container_type::value_type;
  using _size_type   = typename container_type::size_type;
  abs_pretreatment() = default;

  virtual void clear(const container_type& val)  = 0;
  virtual void insert(const _value_type& val)    = 0;
  virtual void erase(const _value_type& val)     = 0;
  virtual void push_back(const _value_type& val) = 0;
  virtual void resize(_size_type val)            = 0;
  virtual void swap(const container_type& val)   = 0;
};
/**
 * 默认的容器预处理类
 * @tparam container_type
 */
template <class container_type>
class pretreatment : public abs_pretreatment<container_type> {
 public:
  using _value_type = typename container_type::value_type;
  using _size_type  = typename container_type::size_type;
  pretreatment()    = default;

  virtual void clear(const container_type& val){};
  virtual void insert(const _value_type& val){};
  virtual void erase(const _value_type& val){};
  virtual void push_back(const _value_type& val){};
  virtual void resize(_size_type val){};
  virtual void swap(const container_type& val){};
};
}  // namespace details

/**
 * @brief 一个可观察的容器
 * @tparam container_type 容器类型
 *
 * @warning 在信号中更改容器内容, 会导致递归调用, 这是请使用 boost::signals2::shared_connection_block 阻止插槽
 * 或者使用不带 _sig 后缀的函数名称,
 * @warning 不带 _sig 后缀名称的函数同时也不会触发预处理
 */
template <class container_type>
class observable_container : public container_type, public details::no_copy {
  //  pretreatment* _pre;

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

  /**
   * @brief 默认构造函数是没有哦预处理的
   */
  explicit observable_container()
      : container_type(),
        sig_begin_clear(),
        sig_begin_insert(),
        sig_begin_erase(),
        sig_begin_push_back(),
        sig_begin_resize(),
        sig_begin_swap(),
        sig_clear(),
        sig_insert(),
        sig_erase(),
        sig_push_back(),
        sig_resize(),
        sig_swap(){};
  /**
   * @brief
   * @param
   */
  explicit observable_container(const _allocator_type& _al)
      : container_type(_al),
        sig_begin_clear(),
        sig_begin_insert(),
        sig_begin_erase(),
        sig_begin_push_back(),
        sig_begin_resize(),
        sig_begin_swap(),
        sig_clear(),
        sig_insert(),
        sig_erase(),
        sig_push_back(),
        sig_resize(),
        sig_swap(){};
  explicit observable_container(const _size_type _count, const _allocator_type& _al = _allocator_type{})
      : container_type(_count, _al),
        sig_begin_clear(),
        sig_begin_insert(),
        sig_begin_erase(),
        sig_begin_push_back(),
        sig_begin_resize(),
        sig_begin_swap(),
        sig_clear(),
        sig_insert(),
        sig_erase(),
        sig_push_back(),
        sig_resize(),
        sig_swap(){};
  //  explicit observable_container(const )

  /**
   * 清理并发出信号
   */
  void clear_sig() {
    /// 如果是默默人就不进行预处理编译
    //    if constexpr (!std::is_same_v<pretreatment, details::pretreatment<container_type> >)
    //      _pre->clear(*this);
    container_type::clear();
    sig_clear();
  };
  /**
   * @brief 插入值并发射信号
   * @param _where 插入位置迭代器
   * @param _val 插入值迭代器
   * @return 指向插入值的迭代器
   */
  _iterator insert_sig(_const_iterator _where, const _value_type& _val) {
    //    if constexpr (!std::is_same_v<pretreatment, details::pretreatment<container_type> >)
    //      _pre->insert(_val);
    sig_begin_insert(_val);
    auto _k_where = container_type::insert(_where, _val);
    sig_insert(*_k_where);
    return _k_where;
  };

  _iterator insert_sig(_const_iterator _where, _value_type&& _val) {
    //    if constexpr (!std::is_same_v<pretreatment, details::pretreatment<container_type> >)
    //      _pre->insert(_val);
    sig_begin_insert(_val);
    auto _k_where = container_type::insert(_where, std::move(_val));
    sig_insert(*_k_where);
    return _k_where;
  };
  /**
   * @brief 擦除值
   * @param _where 值的位置
   * @return 被擦除的迭代器的位置
   */
  _iterator erase_sig(_iterator _where) {
    auto k_val = *_where;
    sig_begin_erase(k_val);
    auto _k_where = container_type::erase(_where);
    //    if constexpr (!std::is_same_v<pretreatment, details::pretreatment<container_type> >)
    //      _pre->erase(*_k_where);
    sig_erase(k_val);
    return _k_where;
  };

  _iterator erase_sig(_const_iterator _where) {
    auto k_val = *_where;
    sig_begin_insert(k_val);
    auto _k_where = container_type::erase(_where);
    //    if constexpr (!std::is_same_v<pretreatment, details::pretreatment<container_type> >)
    //      _pre->erase(*_k_where);
    sig_erase(k_val);
    return _k_where;
  }

  _iterator erase_sig(_const_reference _val) {
    auto it = std::find(container_type::begin(), container_type::end(), _val);
    if (it == container_type::end())
      throw std::runtime_error{"在容器内找不到值"};

    return erase_sig(it);
  }

  /**
   * 向后方追加值
   * @param _val 值
   */
  void push_back_sig(_const_reference _val) {
    //    if constexpr (!std::is_same_v<pretreatment, details::pretreatment<container_type> >)
    //      _pre->push_back(_val);
    sig_begin_push_back(_val);
    container_type::push_back(_val);
    sig_push_back(container_type::back());
  };

  void push_back_sig(_value_type&& _val) {
    //    if constexpr (!std::is_same_v<pretreatment, details::pretreatment<container_type> >)
    //      _pre->push_back(_val);
    sig_begin_push_back(_val);
    container_type::push_back(std::move(_val));
    sig_push_back(container_type::back());
  };

  /**
   * @brief 调整大小并发出型号
   * @param _count 大小
   */
  void resize_sig(_size_type _count) {
    //    if constexpr (!std::is_same_v<pretreatment, details::pretreatment<container_type> >)
    //      _pre->resize(_count);
    sig_begin_resize(_count);
    container_type::resize(_count);
    sig_resize(_count);
  }
  /**
   * @brief 交换容器内的内容
   * @param other 另一个容器
   */
  void swap_sig(container_type& other) {
    //    if constexpr (!std::is_same_v<pretreatment, details::pretreatment<container_type> >)
    //      _pre->swap(other);
    sig_begin_swap(other);
    container_type::swap(other);
    sig_swap(*this);
  }
  template <class Fun>
  void sort_sig(Fun fun) {
    sig_begin_sort(*this);
    std::sort(container_type::begin(), container_type::end(), fun);
    sig_end_sort(*this);
  }

  void sort_sig() {
    sig_begin_sort(*this);
    if constexpr (details::is_smart_pointer<_value_type>::value)
      std::sort(container_type::begin(), container_type::end(), [](_const_reference r, _const_reference l) {
        return *r < *l;
      });
    else
      std::sort(container_type::begin(), container_type::end(), [](_const_reference r, _const_reference l) {
        return r < l;
      });
    sig_end_sort(*this);
  }

  void sort() {
    if constexpr (details::is_smart_pointer<_value_type>::value)
      std::sort(container_type::begin(), container_type::end(), [](_const_reference r, _const_reference l) {
        return *r < *l;
      });
    else
      std::sort(container_type::begin(), container_type::end(), [](_const_reference r, _const_reference l) {
        return r < l;
      });
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
  /**
   * @warning 在信号中更改容器内容, 会导致递归调用, 这是请使用 boost::signals2::shared_connection_block 阻止插槽
   * 或者使用不带 _sig 后缀的函数名称,
   */

  /**
   * @brief 开始清理时发出的信号
   */
  boost::signals2::signal<void()> sig_begin_clear;
  /**
   * @brief 开始插入时发出的信号
   */
  boost::signals2::signal<void(const _value_type& val)> sig_begin_insert;
  /**
   * @brief 开始擦除时发出的信号
   */
  boost::signals2::signal<void(const _value_type& val)> sig_begin_erase;
  /**
   * @brief 开始追加时发出的信号
   */
  boost::signals2::signal<void(const _value_type& val)> sig_begin_push_back;
  /**
   * @brief 开始调整大小时发出的信号
   */
  boost::signals2::signal<void(_size_type where)> sig_begin_resize;
  /**
   * @brief 开始交换容器内容时发出的信号
   */
  boost::signals2::signal<void(const container_type& val)> sig_begin_swap;

  /**
   * @brief 排序开始回调
   *
   */
  boost::signals2::signal<void(const container_type& val)> sig_begin_sort;

  //----------------------------------------------------------------------------

  /**
   * @brief 清理时发出的信号
   */
  boost::signals2::signal<void()> sig_clear;
  /**
   * @brief 插入时发出的信号
   */
  boost::signals2::signal<void(const _value_type& val)> sig_insert;
  /**
   * @brief 擦除时发出的信号
   */
  boost::signals2::signal<void(const _value_type& val)> sig_erase;
  /**
   * @brief 追加时发出的信号
   */
  boost::signals2::signal<void(const _value_type& val)> sig_push_back;
  /**
   * @brief 调整大小时发出的信号
   */
  boost::signals2::signal<void(_size_type where)> sig_resize;
  /**
   * @brief 交换容器内容时发出的信号
   */
  boost::signals2::signal<void(const container_type& val)> sig_swap;

  /**
   * @brief 排序回调
   *
   */
  boost::signals2::signal<void(const container_type& val)> sig_sort;

  using container_type::operator=;
  using container_type::operator[];
};
// using my_str = observable_container<std::vector<std::string> >;
// using mt_map = observable_container<std::map<std::string, std::string> >;
}  // namespace doodle
