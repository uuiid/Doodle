#pragma once

#include <doodle_core/doodle_core_fwd.h>

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
#include <variant>
#include <vector>

namespace doodle::orm {
template <typename Table>
struct object_t {
  using table_type = Table;
  operator std::type_index() const { return std::type_index{typeid(Table)}; }
};

template <typename Table>
auto object() {
  return object_t<Table>{};
}
namespace detail {
template <typename T>
struct select_arg_type {
  using type = std::decay_t<T>;
};

template <typename C, typename T>
struct select_arg_type<T C::*> {
  using type = std::decay_t<T>;
};

template <typename Table>
struct select_arg_type<object_t<Table>> {
  using type = Table;
};

template <typename T>
using select_arg_type_t = typename select_arg_type<std::decay_t<T>>::type;

}  // namespace detail

template <typename... TableColumns>
struct select_result_type;

struct select_t {
 private:
  friend class storage;

  template <typename... TableColumns>
  friend auto select(TableColumns... in_columns) -> select_result_type<detail::select_arg_type_t<TableColumns>...>;
  struct join_info_t {
    std::type_index join_table_type_index_{typeid(void)};
    join_type type_{join_type::inner};
    std::function<std::pair<std::string, std::string>(const storage&)> on_condition_fun_;
  };

  struct where_info_t {
    std::function<std::string(const storage&)> condition_fun_{[](const storage&) { return ""; }};
    std::function<void(sqlite_stmt&)> bind_fun_{[](sqlite_stmt&) {}};
  };

  struct order_by_info_t {
    bool ascending_{true};
    std::function<std::string(const storage&)> column_name_fun_;

    std::string operator()(const storage& s) const {
      return fmt::format("{}{}", column_name_fun_(s), ascending_ ? "" : " DESC");
    }
  };

  // 结果类型
  std::function<std::vector<std::string>(const storage&)> get_column_names_fun_;
  std::type_index from_table_type_index_{typeid(void)};
  std::vector<join_info_t> joins_;
  where_info_t wheres_;
  std::vector<order_by_info_t> order_bys_;
  std::optional<std::size_t> limit_;
  std::optional<std::size_t> offset_;

 public:
  template <typename FromTable>
  select_t& from() {
    from_table_type_index_ = std::type_index{typeid(FromTable)};
    return *this;
  }

  template <typename JoinTable>
  select_t& join(auto in_ptr, auto in_ref_ptr, join_type in_join_type = join_type::inner) {
    static_assert(std::is_member_pointer_v<decltype(in_ptr)>, "join条件必须是成员指针");
    static_assert(std::is_member_pointer_v<decltype(in_ref_ptr)>, "join条件必须是成员指针");
    // using JoinTableType = typename std::decay_t<decltype(JoinTable::table_type)>;
    join_info_t join_info{};
    join_info.join_table_type_index_ = std::type_index{typeid(JoinTable)};
    join_info.type_                  = in_join_type;
    join_info.on_condition_fun_      = [in_ptr, in_ref_ptr](const storage& s) {
      return std::make_pair(s.get_column_name(in_ptr), s.get_column_name(in_ref_ptr));
    };
    joins_.push_back(std::move(join_info));
    return *this;
  }

  template <typename T>
    requires(is_column_operations_specialization_v<T>)
  select_t& where(T&& condition_fun) {
    auto l_condition_fun_ptr = std::make_shared<T>(std::forward<T>(condition_fun));
    wheres_.condition_fun_   = [l_condition_fun_ptr](const storage& s) {
      return fmt::format("WHERE {}", l_condition_fun_ptr->to_sql(s));
    };
    wheres_.bind_fun_ = [l_condition_fun_ptr](sqlite_stmt& stmt) { l_condition_fun_ptr->bind(stmt); };
    return *this;
  }
  template <typename T>
  select_t& order_by(auto T::* in_column_fun, bool ascending = true) {
    order_by_info_t order_by_info{};
    order_by_info.ascending_       = ascending;
    order_by_info.column_name_fun_ = [in_column_fun](const storage& s) { return s.get_column_name(in_column_fun); };
    order_bys_.push_back(std::move(order_by_info));
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
};
template <typename... TableColumns>
struct select_result_type_iterator {
  using type              = std::tuple<std::decay_t<TableColumns>...>;

  using iterator_type     = select_result_type_iterator<TableColumns...>;
  using iterator_category = std::input_iterator_tag;
  using value_type        = type;
  using difference_type   = std::ptrdiff_t;
  using pointer           = const value_type*;
  using reference         = const value_type&;

  storage* s_;
  std::shared_ptr<sqlite_stmt> stmt_;
  bool is_end_{true};
  mutable std::shared_ptr<value_type> cache_;

  explicit select_result_type_iterator(storage& s, const std::shared_ptr<sqlite_stmt>& stmt)
      : s_(&s), stmt_(stmt), is_end_(false), cache_(std::make_shared<value_type>()) {}
  select_result_type_iterator()  = default;
  ~select_result_type_iterator() = default;

  // 从sqlite_stmt中提取数据并转换为type类型
  type get() const;
  void next() {
    if (is_end_ || !stmt_) return is_end_ = true, void();

    const auto l_rc = sqlite3_step(stmt_->stmt_);
    if (l_rc == SQLITE_ROW) return is_end_ = false, void();
    if (l_rc == SQLITE_DONE) return is_end_ = true, void();

    is_end_ = true;
    DOODLE_ORM_ERROR_SQLITE3(l_rc, stmt_->db_);
  }

  // 实现迭代器接口
  iterator_type begin() const {
    iterator_type l_iter{*this};
    l_iter.next();
    return l_iter;
  }

  iterator_type begin() { return static_cast<const iterator_type&>(*this).begin(); }

  iterator_type end() const { return {}; }

  iterator_type end() { return static_cast<const iterator_type&>(*this).end(); }

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
struct select_result_type : select_t {
  using type      = std::tuple<std::decay_t<TableColumns>...>;
  using view_type = select_result_type_iterator<TableColumns...>;
  // using result_type = std::tuple<std::decay_t<TableColumns>...>;

  template <typename Table>
  auto from() {
    select_t::from<Table>();
    return *this;
  }
  template <typename JoinTable>
  auto join(auto in_ptr, auto in_ref_ptr, join_type in_join_type = join_type::inner) {
    select_t::join<JoinTable>(in_ptr, in_ref_ptr, in_join_type);
    return *this;
  }
  template <typename T>
    requires(is_column_operations_specialization_v<T>)
  auto where(T&& condition_fun) {
    select_t::where(std::forward<T>(condition_fun));
    return *this;
  }
  template <typename T>
  auto order_by(auto T::* in_column_fun, bool ascending = true) {
    select_t::order_by(in_column_fun, ascending);
    return *this;
  }
  auto limit(std::size_t count) {
    select_t::limit(count);
    return *this;
  }
  auto offset(std::size_t count) {
    select_t::offset(count);
    return *this;
  }
};

// 将select(&Table::column1, &Table::column2, object<Table2>()) 转换为
// select_t<uuid, std::string, Table2>
template <typename... TableColumns>
auto select(TableColumns... in_columns) -> select_result_type<detail::select_arg_type_t<TableColumns>...> {
  static_assert(sizeof...(TableColumns) > 0, "至少需要选择一个列");
  select_result_type<detail::select_arg_type_t<TableColumns>...> l_ret{};
  l_ret.get_column_names_fun_ = [columns = std::make_tuple(in_columns...)](const storage& s) {
    std::vector<std::string> column_names;
    auto l_iter_fun = [&s, &column_names](auto&& column) {
      // 处理每个参数
      // 如果是成员指针，获取列名
      if constexpr (std::is_member_pointer_v<std::decay_t<decltype(column)>>) {
        column_names.push_back(s.get_column_name(column));
      } else if constexpr (std::is_same_v<
                               std::decay_t<decltype(column)>, object_t<detail::select_arg_type_t<decltype(column)>>>) {
        // 如果是object<Table>，获取表的所有列名
        using table_type        = detail::select_arg_type_t<decltype(column)>;
        auto table_column_names = s.get_table_column_names<table_type>();
        column_names.insert(column_names.end(), table_column_names.begin(), table_column_names.end());
      } else {
        static_assert(always_false<decltype(column)>, "不支持的参数类型");
      }
    };

    std::apply([&](auto&&... column) { (l_iter_fun(column), ...); }, columns);
    return column_names;
  };
  return l_ret;
}

}  // namespace doodle::orm