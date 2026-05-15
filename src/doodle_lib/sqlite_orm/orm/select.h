#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/alias.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <fmt/format.h>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
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

struct select_t {
 protected:
  friend class storage;
  friend select_t select(storage& s);

  struct join_info_t {
    std::string join_table_name_;
    join_type type_{join_type::inner};
    std::pair<std::string, std::string> condition_;
    column_info_ptr self_column_info_;
    column_info_ptr join_column_info_;
  };

  // 结果类型
  std::vector<column_info_ptr> column_names_;
  std::string from_table_name_;
  std::vector<join_info_t> joins_;
  std::shared_ptr<column_operations_base_t> wheres_;
  std::vector<std::string> order_bys_;
  std::optional<std::size_t> limit_;
  std::optional<std::size_t> offset_;
  storage* s_{nullptr};
  std::shared_ptr<sqlite_stmt> stmt_;

  std::vector<std::shared_ptr<storage_column_variant>> bind_variants_{};

  void run();

 public:
  explicit select_t(storage& s) : s_(&s) {}
  template <typename FromTable>
  select_t& from() {
    from_table_name_ = s_->get_table_name<FromTable>();
    return *this;
  }

  template <typename JoinTable>
  select_t& join(auto in_ptr, auto in_ref_ptr, join_type in_join_type = join_type::inner)
    requires(
        (std::is_member_pointer_v<decltype(in_ptr)> || is_alias_column_info_specialization_v<decltype(in_ptr)>) &&
        (std::is_member_pointer_v<decltype(in_ref_ptr)> || is_alias_column_info_specialization_v<decltype(in_ref_ptr)>)
    )
  {
    // using JoinTableType = typename std::decay_t<decltype(JoinTable::table_type)>;
    join_info_t join_info{};
    join_info.join_table_name_  = s_->get_table_name<JoinTable>();
    join_info.type_             = in_join_type;
    join_info.condition_        = {s_->get_column_name(in_ptr), s_->get_column_name(in_ref_ptr)};
    join_info.self_column_info_ = std::make_shared<column_info_t<class_type_t<std::decay_t<decltype(in_ptr)>>>>(in_ptr);
    join_info.join_column_info_ =
        std::make_shared<column_info_t<class_type_t<std::decay_t<decltype(in_ref_ptr)>>>>(in_ref_ptr);
    joins_.push_back(std::move(join_info));
    return *this;
  }

  template <typename T>
  select_t& where(T&& condition_fun);

  template <typename T>
  select_t& order_by(auto T::* in_column_fun, bool ascending = true) {
    order_bys_.push_back(s_->get_column_name(in_column_fun) + (ascending ? "" : " DESC"));
    return *this;
  }
  select_t& limit(std::size_t count) {
    limit_ = count;
    return *this;
  }
  select_t& offset(std::size_t count) {
    offset_ = count;
    return *this;
  }

  std::string to_sql(const storage& s) const;

  void collect_bind_variants(std::vector<std::shared_ptr<storage_column_variant>>& bind_variants) const;
  template <typename... TableColumns>
  select_t& columns_(TableColumns... in_columns);

  // 约束TableColumnsTuple 必须是一个tuple, 并且tuple的元素必须是成员指针或者object<Table>
  template <is_tuple_of_columns TableColumnsTuple>
  select_t& columns(TableColumnsTuple&& in_columns_tuple) {
    std::apply(
        [this](auto&&... columns) { columns_(std::forward<decltype(columns)>(columns)...); },
        std::forward<TableColumnsTuple>(in_columns_tuple)
    );
    return *this;
  }

  template <typename... TableColumns>
  struct result_type_iterator {
    using type              = std::tuple<class_result_type_t<std::decay_t<TableColumns>>...>;

    using iterator_type     = result_type_iterator<TableColumns...>;
    using iterator_category = std::input_iterator_tag;
    using value_type        = type;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const value_type*;
    using reference         = const value_type&;

    storage* s_;
    std::shared_ptr<sqlite_stmt> stmt_;
    bool is_end_{true};
    mutable std::shared_ptr<value_type> cache_;

    explicit result_type_iterator(storage& s, const std::shared_ptr<sqlite_stmt>& stmt)
        : s_(&s), stmt_(stmt), is_end_(false), cache_(std::make_shared<value_type>()) {
      next();
    }
    result_type_iterator()  = default;
    ~result_type_iterator() = default;

    // 从sqlite_stmt中提取数据并转换为type类型
    type get() const;
    void next() {
      if (is_end_ || !stmt_) return is_end_ = true, void();

      const auto l_rc = stmt_->step_not_throw();
      if (l_rc == SQLITE_ROW) return is_end_ = false, void();
      if (l_rc == SQLITE_DONE) return is_end_ = true, void();

      is_end_ = true;
      DOODLE_ORM_ERROR_SQLITE3(l_rc, stmt_->db_);
    }

    reference operator*() const {
      if (is_end_) throw std::out_of_range("Dereferencing end iterator");
      *cache_ = get();
      return *cache_;
    }

    pointer operator->() const { return std::addressof(operator*()); }

    iterator_type& operator++() {
      next();
      return *this;
    }

    iterator_type operator++(int) {
      iterator_type l_old{*this};
      ++(*this);
      return l_old;
    }

    bool operator==(const iterator_type& rhs) const {
      if (is_end_ && rhs.is_end_) return true;
      return s_ == rhs.s_ && stmt_.get() == rhs.stmt_.get() && is_end_ == rhs.is_end_;
    }

    bool operator!=(const iterator_type& rhs) const { return !(*this == rhs); }
  };
  template <typename... TableColumns>
  struct result_type_t {
    select_t& select_;
    using type          = std::tuple<class_result_type_t<std::decay_t<TableColumns>>...>;
    using iterator_type = result_type_iterator<TableColumns...>;

    auto begin() {
      if (!select_.s_ || !select_.stmt_) return end();
      return iterator_type{*select_.s_, select_.stmt_};
    }
    auto end() { return iterator_type{}; }
    auto begin() const {
      if (!select_.s_ || !select_.stmt_) return end();
      return iterator_type{*select_.s_, select_.stmt_};
    }
    auto end() const { return iterator_type{}; }
  };

  template <typename TableColumnsTuple>
  struct result_type_t_helper;
  template <typename... TableColumns>
  struct result_type_t_helper<std::tuple<TableColumns...>> {
    using type = result_type_t<TableColumns...>;
  };

  // 提取tuple的类型并返回一个可迭代的结果类型
  template <is_tuple_of_columns TableColumnsTuple>
  result_type_t_helper<std::decay_t<TableColumnsTuple>>::type get_result(TableColumnsTuple&& in_columns_tuple) {
    // 避免未使用参数的编译警告
    (void)in_columns_tuple;
    run();
    return typename result_type_t_helper<std::decay_t<TableColumnsTuple>>::type{*this};
  }
  template <is_tuple_of_columns TableColumnsTuple>
  result_type_t_helper<std::decay_t<TableColumnsTuple>>::type operator()(TableColumnsTuple&& in_columns_tuple) {
    (void)in_columns_tuple;
    run();
    return typename result_type_t_helper<std::decay_t<TableColumnsTuple>>::type{*this};
  }

  template <typename... TableColumns>
  result_type_t<TableColumns...> operator()(TableColumns&&... in_columns) {
    columns_(std::forward<TableColumns>(in_columns)...);
    run();
    return result_type_t<TableColumns...>{*this};
  }
};

inline select_t select(storage& s) {
  select_t l_ret{s};
  return l_ret;
}
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