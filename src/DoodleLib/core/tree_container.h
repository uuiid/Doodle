//
// Created by TD on 2021/7/30.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <boost/hana.hpp>
#include <boost/intrusive/intrusive_fwd.hpp>
#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/pack_options.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/trivial_value_traits.hpp>
#include <boost/signals2.hpp>

namespace doodle {

using set_base_hook = boost::intrusive::set_base_hook<
    boost::intrusive::link_mode<boost::intrusive::safe_link>>;

class tree_node;
using tree_node_ptr = std::shared_ptr<tree_node>;

template <class... type_arg>
tree_node_ptr make_tree(type_arg&&... in_arg);

namespace details {
class DOODLELIB_API tree_node_destroy : public std::default_delete<tree_node> {
 public:
  tree_node_destroy() = default;
  void operator()(tree_node* in_ptr);
};

template <class container>
class DOODLELIB_API observe : public no_copy {
 public:
  using _value_type    = typename container::value_type;
  using container_type = container;
  observe()            = default;

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
  boost::signals2::signal<void(std::size_t where)> sig_begin_resize;
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
  boost::signals2::signal<void(std::size_t where)> sig_resize;
  /**
   * @brief 交换容器内容时发出的信号
   */
  boost::signals2::signal<void(const container_type& val)> sig_swap;

  /**
   * @brief 排序回调
   *
   */
  boost::signals2::signal<void(const container_type& val)> sig_sort;
};
}  // namespace details

/**
 * @brief 这是一个树结构
 */
class DOODLELIB_API tree_node : public std::enable_shared_from_this<tree_node>,
                                public set_base_hook,
                                public details::no_copy {
 public:
  using tree_node_ptr = std::shared_ptr<tree_node>;

  using child_set = boost::intrusive::set<
      tree_node,
      boost::intrusive::base_hook<tree_node>>;
  // boost::intrusive::constant_time_size<false>
  using child_set_owner    = std::vector<tree_node_ptr>;
  using signal_observe     = details::observe<child_set>;
  using signal_observe_ptr = std::shared_ptr<signal_observe>;

  using iterator               = typename child_set ::iterator;
  using const_iterator         = typename child_set ::const_iterator;
  using reverse_iterator       = typename child_set ::reverse_iterator;
  using const_reverse_iterator = typename child_set ::const_reverse_iterator;

 private:
  friend details::tree_node_destroy;
  tree_node();
  explicit tree_node(tree_node* in_parent, MetadataPtr in_data);
  explicit tree_node(const tree_node_ptr& in_parent, MetadataPtr in_data);

  iterator insert_private(const tree_node_ptr& in_);
  iterator insert(const tree_node_ptr& in_, bool emit_solt);

  // template <      class T>
  // struct test_value {
  //   constexpr static auto k_has_connect = boost::hana::is_valid(
  //       [](auto&& obj, auto& in_arg) -> decltype(obj->connect(in_arg)) {});

  //   /// 这里我们再测试一下是否需要指向容器的指针，需要的的话添加一下
  //   constexpr static auto k_has_node_ptr = boost::hana::is_valid(
  //       [](auto&& obj) -> decltype(obj->node_ptr) {});

  //   template <class arg_t>
  //   static void connect(const T& data, arg_t& in_arg) {
  //     data->connect(in_arg);
  //   };

  //   template <class arg_t>
  //   static void set_node_ptr(const T& data, arg_t& in_arg) {
  //     data->node_ptr = in_arg;
  //   };
  // };

  // template <
  //     class T,
  //     std::enable_if_t<!details::is_smart_pointer<T>::value, bool> = true>
  // struct test_value {
  //   constexpr static auto k_has_connect = boost::hana::is_valid(
  //       [](auto&& obj, auto& in_arg) -> decltype(obj.connect(in_arg)) {});

  //   /// 这里我们再测试一下是否需要指向容器的指针，需要的的话添加一下
  //   constexpr static auto k_has_node_ptr = boost::hana::is_valid(
  //       [](auto&& obj) -> decltype(obj.node_ptr) {});

  //   template <class arg_t>
  //   static void connect(T& data, arg_t& in_arg) {
  //     data.connect(in_arg);
  //   };

  //   template <class arg_t>
  //   static void set_node_ptr(const T& data, arg_t& in_arg) {
  //     data.node_ptr = in_arg;
  //   };
  // };
  template <class T>
  struct test_value {
    /// 在这里我们测试一下自动连接是否可用， 可用的话进行连接
    constexpr static auto k_has_connect = boost::hana::is_valid(
        [](auto&& obj, auto& in_arg) -> decltype(obj.connect(in_arg)) {});

    /// 这里我们再测试一下是否需要指向容器的指针，需要的的话添加一下
    constexpr static auto k_has_node = boost::hana::is_valid(
        [](auto&& obj) -> decltype(obj.node_ptr) {});

    template <class arg_t, class T>
    static std::enable_if_t<details::is_smart_pointer<T>::value>
    connect(const T& data, arg_t& in_arg) {
      if constexpr (decltype(k_has_connect(*data, in_arg)){})
        data->connect(in_arg);
    };

    template <class arg_t, class T>
    static std::enable_if_t<details::is_smart_pointer<T>::value>
    set_node_ptr(const T& data, arg_t& in_arg) {
      if constexpr (decltype(k_has_node(*data)){})
        data->node_ptr = in_arg;
    };

    template <class arg_t, class T>
    static std::enable_if_t<!details::is_smart_pointer<T>::value>
    connect(T& data, arg_t& in_arg) {
      if constexpr (decltype(k_has_connect(data, in_arg)){})
        data.connect(in_arg);
    };

    template <class arg_t, class T>
    static std::enable_if_t<!details::is_smart_pointer<T>::value>
    set_node_ptr(const T& data, arg_t& in_arg) {
      if constexpr (decltype(k_has_node(data)){})
        data.node_ptr = in_arg;
    };
  };

 public:
  ~tree_node();

  template <class... type_arg>
  static tree_node_ptr make_this(type_arg&&... in_arg) {
    auto tmp = std::shared_ptr<tree_node>{
        new tree_node{std::forward<type_arg>(in_arg)...},
        details::tree_node_destroy()};
    /// 非默认构造, 这个时候可以直接检查父物体是否为空后插入
    if constexpr (sizeof...(in_arg) > 1) {
      if (tmp->has_parent())
        tmp->parent->insert_private(tmp);
    }

    using test_value_t = test_value<decltype(data)>;
    test_value_t::connect(tmp->data, tmp);
    test_value_t::set_node_ptr(tmp->data, tmp);
    return tmp;
  };

  [[nodiscard]] bool is_root() const;
  [[nodiscard]] bool has_parent() const;
  [[nodiscard]] tree_node_ptr get_parent() const;
  [[nodiscard]] const child_set& get_children() const;
  [[nodiscard]] bool empty() const;

  iterator insert(const tree_node_ptr& in_);
  iterator insert(const MetadataPtr& in_ptr);
  iterator insert_sig(const tree_node_ptr& in_);
  iterator insert_sig(const MetadataPtr& in_ptr);

  iterator erase(const tree_node_ptr& in_);
  iterator erase(const MetadataPtr& in_ptr);
  iterator erase_sig(const tree_node_ptr& in_);
  iterator erase_sig(const MetadataPtr& in_ptr);

  void clear();
  void clear_sig();
  signal_observe_ptr get_signal_observe() const;
  /**
   * @brief 这里所有的迭代器都是迭代子项， 没有包括父物体
   * 
   * @return iterator 子项迭代器
   */
  [[nodiscard]] iterator begin() noexcept;
  [[nodiscard]] const_iterator begin() const noexcept;
  [[nodiscard]] iterator end() noexcept;
  [[nodiscard]] const_iterator end() const noexcept;
  [[nodiscard]] reverse_iterator rbegin() noexcept;
  [[nodiscard]] const_reverse_iterator rbegin() const noexcept;
  [[nodiscard]] reverse_iterator rend() noexcept;
  [[nodiscard]] const_reverse_iterator rend() const noexcept;
  [[nodiscard]] const_reverse_iterator crbegin() const noexcept;
  [[nodiscard]] const_reverse_iterator crend() const noexcept;

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
  child_set_owner child_owner;
  details::observe<child_set> sig_class;
  signal_observe_ptr p_sig;
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
