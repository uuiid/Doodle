#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/alias.h>
#include <doodle_lib/sqlite_orm/orm/bind_value.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <concepts>
#include <fmt/format.h>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace doodle::orm {
template <typename... TableColumns>
struct select_result_type;
template <typename TableColumnsTuple>
concept is_tuple_of_columns = requires(TableColumnsTuple t) {
  // using columns_tuple_type = std::decay_t<TableColumnsTuple>;
  std::tuple_size_v<std::decay_t<TableColumnsTuple>>;
  // std::apply(
  //     [](auto&&... columns) {
  //       ((std::is_member_pointer_v<std::decay_t<decltype(columns)>> ||
  //         is_object_specialization_v<std::decay_t<decltype(columns)>>) &&
  //        ...);
  //     },
  //     t
  // );
};

template <typename... Columns>
struct select_template_t;

template <bool IsSingleColumn, typename T, typename Source>
concept result_vector_value_constructible =
    (IsSingleColumn && std::constructible_from<T, Source>) ||
    (!IsSingleColumn && requires(Source&& source) { std::make_from_tuple<T>(std::forward<Source>(source)); });

struct select_t {
 protected:
  friend class storage;
  friend select_t select(storage& s);

  struct join_info_t {
    join_type type_{join_type::inner};
    table_info_base_ptr join_table_info_;
    column_info_ptr self_column_info_;
    column_info_ptr join_column_info_;
    std::shared_ptr<on_operations> on_condition_;
  };
  struct order_by_info_t {
    column_info_ptr column_info_;
    bool ascending_{true};
  };

  struct impl_t {
    // 结果类型
    std::vector<column_info_ptr> column_names_;
    std::string from_table_name_;
    std::vector<join_info_t> joins_;
    std::shared_ptr<column_operations_base_t> wheres_;
    std::vector<order_by_info_t> order_bys_;
    std::optional<std::size_t> limit_;
    std::optional<std::size_t> offset_;
    std::vector<column_info_ptr> group_bys_;
    storage* s_{nullptr};
    std::shared_ptr<sqlite_stmt> stmt_;
    bind_value_collector_t bind_variants_{};
  };

  std::shared_ptr<impl_t> impl_;

  void run();

 public:
  explicit select_t(storage& s) : impl_(std::make_shared<impl_t>()) { impl_->s_ = &s; }
  template <typename FromTable>
  select_t from();
  template <typename FromTable>
  select_t join(auto in_ptr, auto in_ref_ptr, join_type in_join_type = join_type::inner)
    requires((std::is_member_pointer_v<decltype(in_ptr)>) && (std::is_member_pointer_v<decltype(in_ref_ptr)>));
  template <typename JoinTable>
  select_t join(JoinTable&& join_table, auto in_ptr, auto in_ref_ptr, join_type in_join_type = join_type::inner)
    requires(
        (std::is_member_pointer_v<decltype(in_ptr)> || is_alias_column_t_v<std::decay_t<decltype(in_ptr)>>) &&
        (std::is_member_pointer_v<decltype(in_ref_ptr)> || is_alias_column_t_v<std::decay_t<decltype(in_ref_ptr)>>) &&

        (is_alias_column_t_v<std::decay_t<decltype(in_ptr)>> ||
         is_alias_column_t_v<std::decay_t<decltype(in_ref_ptr)>>) &&
        is_alias_t_v<JoinTable>
    );
  template <typename FromTable>
  select_t join(const on_operations& in_on, join_type in_join_type = join_type::inner);
  template <typename JoinTable>
  select_t join(JoinTable&& join_table, const on_operations& in_on, join_type in_join_type = join_type::inner)
    requires is_alias_t_v<JoinTable>;

  template <typename T>
  select_t where(T&& condition_fun);

  template <typename T>
  select_t order_by(auto T::* in_column_fun, bool ascending = true);
  // 别名的 order by
  template <typename T>
    requires is_alias_column_t_v<std::decay_t<T>>
  select_t order_by(T&& alias_column, bool ascending = true);

  select_t limit(std::size_t count) {
    impl_->limit_ = count;
    return *this;
  }
  select_t offset(std::size_t count) {
    impl_->offset_ = count;
    return *this;
  }
  template <typename... TableColumns>
  select_t group_by(auto TableColumns::*... in_columns);

  std::string to_sql(const storage& s) const;

  void collect_bind_variants(bind_value_collector_t& bind_variants) const;
  template <typename... TableColumns>
  select_template_t<TableColumns...> columns(TableColumns... in_columns);

  // 辅助类模板, 如果传入的模板参数个数是 1 个, 则直接返回该类型, 否则返回一个 tuple 包裹的类型
  template <typename... TableColumns>
  struct class_result_type {
    using type = std::conditional_t<
        sizeof...(TableColumns) == 1,
        class_result_type_t<std::decay_t<std::tuple_element_t<0, std::tuple<TableColumns...>>>>,
        std::tuple<class_result_type_t<std::decay_t<TableColumns>>...>>;
  };
  template <typename... TableColumns>
  using result_type = typename class_result_type<TableColumns...>::type;
  template <typename... TableColumns>
  struct result_type_iterator {
    using type              = result_type<TableColumns...>;

    using iterator_type     = result_type_iterator<TableColumns...>;
    using iterator_category = std::input_iterator_tag;
    using value_type        = type;
    using difference_type   = std::ptrdiff_t;
    using pointer           = value_type*;
    using reference         = value_type&;
    select_t* select_;
    bool is_end_{true};
    mutable std::shared_ptr<value_type> cache_;
    result_type_iterator() = default;
    explicit result_type_iterator(select_t& in_select);

    ~result_type_iterator() = default;

    // 从sqlite_stmt中提取数据并转换为type类型
    type get() const;
    void next();

    reference operator*() const;

    pointer operator->() const;

    iterator_type& operator++();

    iterator_type operator++(int);

    bool operator==(const iterator_type& rhs) const;

    bool operator!=(const iterator_type& rhs) const;
  };
  template <typename... TableColumns>
  struct result_type_t;
};

template <typename... TableColumns>
struct select_t::result_type_t {
  select_t select_;
  using type          = result_type<TableColumns...>;
  using iterator_type = result_type_iterator<TableColumns...>;

  iterator_type begin();
  iterator_type end();
  iterator_type begin() const;
  iterator_type end() const;

  // to vector
  std::vector<type> to_vector();
  // 单列时 T 必须可以直接从 type 构造; 多列时必须可以通过 std::make_from_tuple 构造
  template <typename T>
    requires(result_vector_value_constructible<sizeof...(TableColumns) == 1, T, type>)
  std::vector<T> to_vector() {
    std::vector<T> l_result{};
    for (auto& item : *this) {
      if constexpr (sizeof...(TableColumns) == 1)
        l_result.push_back(T{item});
      else
        l_result.push_back(std::make_from_tuple<T>(item));
    }
    return l_result;
  }
  // to set
  std::set<type> to_set();

  // to single value, 如果结果集有多于1行, 则抛出异常
  type to_single();
  // to optional
  std::optional<type> to_optional();
};

inline select_t select(storage& s) {
  select_t l_ret{s};
  return l_ret;
}

template <typename... Columns>
struct select_template_t : public select_t {
  // 这个类的作用是为了支持在编译期就确定结果类型的select, 通过模板参数传入列信息, 从而在编译期就能推断出结果类型,
  // 避免了运行时的类型推断开销
  using select_t::select_t;
  explicit select_template_t(select_t&& select) : select_t(std::move(select)) {}
  template <typename FormTable>
  select_template_t from();
  template <typename FromTable>
  select_template_t join(auto in_ptr, auto in_ref_ptr, join_type in_join_type = join_type::inner);
  template <typename JoinTable>
  select_template_t join(
      JoinTable&& join_table, auto in_ptr, auto in_ref_ptr, join_type in_join_type = join_type::inner
  );
  template <typename FromTable>
  select_template_t join(const on_operations& in_on, join_type in_join_type = join_type::inner) {
    select_t::join(in_on, in_join_type);
    return *this;
  }
  template <typename JoinTable>
  select_template_t join(JoinTable&& join_table, const on_operations& in_on, join_type in_join_type = join_type::inner)
    requires is_alias_t_v<std::decay_t<JoinTable>>
  {
    select_t::join(std::forward<JoinTable>(join_table), in_on, in_join_type);
    return *this;
  }

  template <typename FormTable>
  select_template_t left_outer_join(auto in_ptr, auto in_ref_ptr) {
    return join<FormTable>(in_ptr, in_ref_ptr, join_type::left);
  }
  template <typename JoinTable>
  select_template_t left_outer_join(JoinTable&& join_table, auto in_ptr, auto in_ref_ptr) {
    return join(std::forward<JoinTable>(join_table), in_ptr, in_ref_ptr, join_type::left);
  }
  template <typename FormTable>
  select_template_t left_outer_join(const on_operations& in_on) {
    return join<FormTable>(in_on, join_type::left);
  }
  template <typename JoinTable>
  select_template_t left_outer_join(JoinTable&& join_table, const on_operations& in_on) {
    return join(std::forward<JoinTable>(join_table), in_on, join_type::left);
  }
  template <typename FormTable>
  select_template_t right_outer_join(auto in_ptr, auto in_ref_ptr) {
    return join<FormTable>(in_ptr, in_ref_ptr, join_type::right);
  }
  template <typename JoinTable>
  select_template_t right_outer_join(JoinTable&& join_table, auto in_ptr, auto in_ref_ptr) {
    return join(std::forward<JoinTable>(join_table), in_ptr, in_ref_ptr, join_type::right);
  }
  template <typename FormTable>
  select_template_t right_outer_join(const on_operations& in_on) {
    return join<FormTable>(in_on, join_type::right);
  }
  template <typename JoinTable>
  select_template_t right_outer_join(JoinTable&& join_table, const on_operations& in_on) {
    return join(std::forward<JoinTable>(join_table), in_on, join_type::right);
  }

  template <typename T>
  select_template_t where(T&& condition_fun);
  template <typename T>
  select_template_t order_by(auto T::* in_column_fun, bool ascending = true);
  template <typename T>
    requires is_alias_column_t_v<std::decay_t<T>>
  select_template_t order_by(T&& alias_column, bool ascending = true);

  select_template_t limit(std::size_t count);
  select_template_t offset(std::size_t count);
  template <typename... TableColumns>
  select_template_t group_by(auto TableColumns::*... in_columns);

  result_type_t<Columns...> operator()();
};

template <typename... TableColumns>
struct select_and_columns_helper {
  select_t select_;
  std::tuple<TableColumns...> columns_tuple_;

  template <typename... InColumns>
  select_and_columns_helper(select_t select, InColumns&&... in_columns)
      : select_(std::move(select)), columns_tuple_(std::forward<InColumns>(in_columns)...) {}
};

template <typename... TableColumns>
select_and_columns_helper<TableColumns...> make_select_column(storage& s, TableColumns... in_columns) {
  return select_and_columns_helper<TableColumns...>{select_t{s}, std::forward<TableColumns>(in_columns)...};
}

}  // namespace doodle::orm