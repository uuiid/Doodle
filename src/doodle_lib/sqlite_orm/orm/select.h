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
#include <vector>

namespace doodle::orm {
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
 protected:
  friend class storage;
  friend select_t select(storage& s);

  template <typename... TableColumns>
  friend auto select(storage& s, TableColumns... in_columns)
      -> select_result_type<detail::select_arg_type_t<TableColumns>...>;
  struct join_info_t {
    std::string join_table_name_;
    join_type type_{join_type::inner};
    std::pair<std::string, std::string> condition_;
  };

  // 结果类型
  std::vector<std::string> column_names_;
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
  template <typename FromTable>
  select_t& from() {
    from_table_name_ = s_->get_table_name<FromTable>();
    return *this;
  }

  template <typename JoinTable>
  select_t& join(auto in_ptr, auto in_ref_ptr, join_type in_join_type = join_type::inner) {
    static_assert(std::is_member_pointer_v<decltype(in_ptr)>, "join条件必须是成员指针");
    static_assert(std::is_member_pointer_v<decltype(in_ref_ptr)>, "join条件必须是成员指针");
    // using JoinTableType = typename std::decay_t<decltype(JoinTable::table_type)>;
    join_info_t join_info{};
    join_info.join_table_name_ = s_->get_table_name<JoinTable>();
    join_info.type_            = in_join_type;
    join_info.condition_       = {s_->get_column_name(in_ptr), s_->get_column_name(in_ref_ptr)};
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
  template <typename TableColumnsStruct>
  select_t& select(TableColumnsStruct&& in_struct) {
    auto l_iter_fun = [this](auto&& column) {
      // 处理每个参数
      // 如果是成员指针，获取列名
      if constexpr (std::is_member_pointer_v<std::decay_t<decltype(column)>>) {
        column_names_.push_back(s_->get_column_name(column));
      } else if constexpr (std::is_same_v<
                               std::decay_t<decltype(column)>, object_t<detail::select_arg_type_t<decltype(column)>>>) {
        // 如果是object<Table>，获取表的所有列名
        using table_type        = detail::select_arg_type_t<decltype(column)>;
        auto table_column_names = s_->get_table_column_names<table_type>();
        column_names_.insert(column_names_.end(), table_column_names.begin(), table_column_names.end());
      } else {
        static_assert(always_false<decltype(column)>, "不支持的参数类型");
      }
    };
    auto l_columns = in_struct.get_select_value();
    std::apply([&](auto&&... column) { (l_iter_fun(column), ...); }, l_columns);
    return *this;
  };
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
      : s_(&s), stmt_(stmt), is_end_(false), cache_(std::make_shared<value_type>()) {
    next();
  }
  select_result_type_iterator()  = default;
  ~select_result_type_iterator() = default;

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
struct select_result_type : select_t {
  using type              = std::tuple<std::decay_t<TableColumns>...>;

  using iterator_type     = select_result_type_iterator<TableColumns...>;
  using iterator_category = std::input_iterator_tag;
  using value_type        = type;
  using difference_type   = std::ptrdiff_t;
  using pointer           = const value_type*;
  using reference         = const value_type&;

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

  auto begin() {
    if (!s_ || !stmt_) return end();
    return iterator_type{*s_, stmt_};
  }
  auto end() { return iterator_type{}; }
  auto begin() const {
    if (!s_ || !stmt_) return end();
    return iterator_type{*s_, stmt_};
  }
  auto end() const { return iterator_type{}; }

  select_result_type& operator()() &;
  select_result_type operator()() &&;
};

template <typename... TableColumns>
auto select(storage& s, TableColumns... in_columns) -> select_result_type<detail::select_arg_type_t<TableColumns>...> {
  static_assert(sizeof...(TableColumns) > 0, "至少需要选择一个列");
  select_result_type<detail::select_arg_type_t<TableColumns>...> l_ret{};
  l_ret.s_        = &s;
  auto l_iter_fun = [&s, &l_ret](auto&& column) {
    // 处理每个参数
    // 如果是成员指针，获取列名
    if constexpr (std::is_member_pointer_v<std::decay_t<decltype(column)>>) {
      l_ret.column_names_.push_back(s.get_column_name(column));
    } else if constexpr (std::is_same_v<
                             std::decay_t<decltype(column)>, object_t<detail::select_arg_type_t<decltype(column)>>>) {
      // 如果是object<Table>，获取表的所有列名
      using table_type        = detail::select_arg_type_t<decltype(column)>;
      auto table_column_names = s.get_table_column_names<table_type>();
      l_ret.column_names_.insert(l_ret.column_names_.end(), table_column_names.begin(), table_column_names.end());
    } else {
      static_assert(always_false<decltype(column)>, "不支持的参数类型");
    }
  };
  (l_iter_fun(in_columns), ...);
  return l_ret;
}

inline select_t select(storage& s) {
  select_t l_ret{};
  l_ret.s_ = &s;
  return l_ret;
}

}  // namespace doodle::orm