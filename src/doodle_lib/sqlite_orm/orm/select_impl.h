#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/alias.h>
#include <doodle_lib/sqlite_orm/orm/alias_subquery.h>
#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/count.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>
#include <doodle_lib/sqlite_orm/orm/table_info.h>

#include <memory>
#include <utility>

namespace doodle::orm {
// select_t 模板方法实现

template <typename FromTable>
select_t select_t::from() {
  impl_->from_table_name_ = std::make_shared<table_info_t>(typeid(FromTable));
  return *this;
}

template <typename FromTable>
select_t select_t::join(auto in_ptr, auto in_ref_ptr, join_type in_join_type)
  requires(
      (std::is_member_pointer_v<decltype(in_ptr)> || is_alias_column_t_v<std::decay_t<decltype(in_ptr)>>) &&
      (std::is_member_pointer_v<decltype(in_ref_ptr)> || is_alias_column_t_v<std::decay_t<decltype(in_ref_ptr)>>)
  )
{
  join_info_t join_info{};
  join_info.type_            = in_join_type;
  join_info.join_table_info_ = std::make_shared<table_info_t>(typeid(FromTable));
  join_info.on_condition_    = std::make_shared<on_operations>(on(c(in_ptr) == c(in_ref_ptr)));
  impl_->joins_.push_back(std::move(join_info));
  return *this;
}

template <typename JoinTable>
select_t select_t::join(JoinTable&& join_table, auto in_ptr, auto in_ref_ptr, join_type in_join_type)
  requires(
      (std::is_member_pointer_v<decltype(in_ptr)> || is_alias_column_t_v<std::decay_t<decltype(in_ptr)>>) &&
      (std::is_member_pointer_v<decltype(in_ref_ptr)> || is_alias_column_t_v<std::decay_t<decltype(in_ref_ptr)>>) &&

      (is_alias_column_t_v<std::decay_t<decltype(in_ptr)>> ||
       is_alias_column_t_v<std::decay_t<decltype(in_ref_ptr)>>) &&
      is_alias_t_v<JoinTable>
  )
{
  join_info_t join_info{};
  join_info.type_            = in_join_type;
  join_info.join_table_info_ = std::make_shared<alias_info_t>(std::forward<JoinTable>(join_table));
  join_info.on_condition_    = std::make_shared<on_operations>(on(c(in_ptr) == c(in_ref_ptr)));
  impl_->joins_.push_back(std::move(join_info));
  return *this;
}

template <typename FromTable>
select_t select_t::join(const on_operations& in_on, join_type in_join_type) {
  join_info_t join_info{};
  join_info.on_condition_    = std::make_shared<on_operations>(in_on);
  join_info.join_table_info_ = std::make_shared<table_info_t>(typeid(FromTable));
  join_info.type_            = in_join_type;
  impl_->joins_.push_back(std::move(join_info));
  return *this;
}
template <typename JoinTable>
select_t select_t::join(JoinTable&& join_table, const on_operations& in_on, join_type in_join_type)
  requires is_alias_t_v<JoinTable>
{
  join_info_t join_info{};
  join_info.on_condition_    = std::make_shared<on_operations>(in_on);
  join_info.join_table_info_ = std::make_shared<alias_info_t>(std::forward<JoinTable>(join_table));
  join_info.type_            = in_join_type;
  impl_->joins_.push_back(std::move(join_info));
  return *this;
}

template <typename T>
select_t select_t::order_by(auto T::* in_column_fun, bool ascending) {
  impl_->order_bys_.push_back(
      {std::make_shared<column_info_t>(std::forward<decltype(in_column_fun)>(in_column_fun)), ascending}
  );
  return *this;
}

template <typename T>
  requires is_alias_column_t_v<std::decay_t<T>>
select_t select_t::order_by(T&& alias_column, bool ascending) {
  impl_->order_bys_.push_back({std::make_shared<alias_column_info_t>(std::forward<T>(alias_column)), ascending});
  return *this;
}

template <typename... TableColumns>
select_t select_t::group_by(auto TableColumns::*... in_columns) {
  impl_->group_bys_ = {std::make_shared<column_info_t>(in_columns)...};
  return *this;
}

// result_type_iterator 模板方法实现

template <typename... TableColumns>
select_t::result_type_iterator<TableColumns...>::result_type_iterator(select_t in_select)
    : select_(in_select), is_end_(false), cache_(std::make_shared<value_type>()) {
  next();
}

template <typename... TableColumns>
void select_t::result_type_iterator<TableColumns...>::next() {
  if (is_end_ || !select_.impl_->stmt_) return is_end_ = true, void();

  const auto l_rc = select_.impl_->stmt_->step_not_throw();
  if (l_rc == SQLITE_ROW) return is_end_ = false, void();
  if (l_rc == SQLITE_DONE) return is_end_ = true, void();

  is_end_ = true;
  DOODLE_ORM_ERROR_SQLITE3(l_rc, select_.impl_->stmt_->db_);
}

template <typename... TableColumns>
typename select_t::result_type_iterator<TableColumns...>::reference
select_t::result_type_iterator<TableColumns...>::operator*() const {
  if (is_end_) throw std::out_of_range("Dereferencing end iterator");
  *cache_ = get();
  return *cache_;
}

template <typename... TableColumns>
typename select_t::result_type_iterator<TableColumns...>::pointer
select_t::result_type_iterator<TableColumns...>::operator->() const {
  return std::addressof(operator*());
}

template <typename... TableColumns>
typename select_t::result_type_iterator<TableColumns...>::iterator_type&
select_t::result_type_iterator<TableColumns...>::operator++() {
  next();
  return *this;
}

template <typename... TableColumns>
typename select_t::result_type_iterator<TableColumns...>::iterator_type
select_t::result_type_iterator<TableColumns...>::operator++(int) {
  iterator_type l_old{*this};
  ++(*this);
  return l_old;
}

template <typename... TableColumns>
bool select_t::result_type_iterator<TableColumns...>::operator==(const iterator_type& rhs) const {
  if (is_end_ && rhs.is_end_) return true;
  return select_.impl_ == rhs.select_.impl_ && is_end_ == rhs.is_end_;
}

template <typename... TableColumns>
bool select_t::result_type_iterator<TableColumns...>::operator!=(const iterator_type& rhs) const {
  return !(*this == rhs);
}

// result_type_t 模板方法实现

template <typename... TableColumns>
select_t::result_type_t<TableColumns...>::iterator_type select_t::result_type_t<TableColumns...>::begin() {
  if (!select_.impl_->s_ || !select_.impl_->stmt_) return end();
  return iterator_type{select_};
}

template <typename... TableColumns>
select_t::result_type_t<TableColumns...>::iterator_type select_t::result_type_t<TableColumns...>::end() {
  return iterator_type{};
}

template <typename... TableColumns>
select_t::result_type_t<TableColumns...>::iterator_type select_t::result_type_t<TableColumns...>::begin() const {
  if (!select_.impl_->s_ || !select_.impl_->stmt_) return end();
  return iterator_type{select_};
}

template <typename... TableColumns>
select_t::result_type_t<TableColumns...>::iterator_type select_t::result_type_t<TableColumns...>::end() const {
  return iterator_type{};
}

template <typename... TableColumns>
std::vector<typename select_t::result_type_t<TableColumns...>::type>
select_t::result_type_t<TableColumns...>::to_vector() {
  std::vector<type> l_result{};
  for (auto& item : *this) {
    l_result.push_back(item);
  }
  return l_result;
}
template <typename... TableColumns>
std::set<typename select_t::result_type_t<TableColumns...>::type> select_t::result_type_t<TableColumns...>::to_set() {
  std::set<type> l_result{};
  for (auto& item : *this) {
    l_result.insert(item);
  }
  return l_result;
}

// template <typename... TableColumns>
// template <typename T>
//   requires(result_vector_value_constructible<
//            sizeof...(TableColumns) == 1, T, typename select_t::result_type_t<TableColumns...>::type>)
// std::vector<T> select_t::result_type_t<TableColumns...>::to_vector()

template <typename... TableColumns>
typename select_t::result_type_t<TableColumns...>::type select_t::result_type_t<TableColumns...>::to_single() {
  auto l_iter = begin();
  if (l_iter == end()) throw std::runtime_error("No rows returned");
  type l_result = *l_iter;
  ++l_iter;
  if (l_iter != end())  // throw std::runtime_error("More than one row returned");
    SPDLOG_DEBUG("More than one row returned, but only the first one will be used");
  return l_result;
}
template <typename... TableColumns>
std::optional<typename select_t::result_type_t<TableColumns...>::type>
select_t::result_type_t<TableColumns...>::to_optional() {
  auto l_iter = begin();
  if (l_iter == end()) return std::nullopt;
  type l_result = *l_iter;
  // ++l_iter;
  // if (l_iter != end()) throw std::runtime_error("More than one row returned");
  return l_result;
}

// select_template_t 模板方法实现

template <typename... Columns>
template <typename FormTable>
select_template_t<Columns...> select_template_t<Columns...>::from() {
  select_t::from<FormTable>();
  return *this;
}

template <typename... Columns>
select_template_t<Columns...> select_template_t<Columns...>::from(subquery_alias_info_t subquery_alias_info) {
  select_t::from(subquery_alias_info);
  return *this;
}
template <typename... Columns>
template <typename FromTable>
select_template_t<Columns...> select_template_t<Columns...>::join(
    auto in_ptr, auto in_ref_ptr, join_type in_join_type
) {
  select_t::join<FromTable>(in_ptr, in_ref_ptr, in_join_type);
  return *this;
}

template <typename... Columns>
template <typename JoinTable>
select_template_t<Columns...> select_template_t<Columns...>::join(
    JoinTable&& join_table, auto in_ptr, auto in_ref_ptr, join_type in_join_type
) {
  select_t::join(std::forward<JoinTable>(join_table), in_ptr, in_ref_ptr, in_join_type);
  return *this;
}

template <typename... Columns>
template <typename T>
select_template_t<Columns...> select_template_t<Columns...>::where(T&& condition_fun) {
  select_t::where(std::forward<T>(condition_fun));
  return *this;
}

template <typename... Columns>
template <typename T>
select_template_t<Columns...> select_template_t<Columns...>::order_by(auto T::* in_column_fun, bool ascending) {
  select_t::order_by(in_column_fun, ascending);
  return *this;
}
template <typename... Columns>
template <typename T>
  requires is_alias_column_t_v<std::decay_t<T>>
select_template_t<Columns...> select_template_t<Columns...>::order_by(T&& alias_column, bool ascending) {
  select_t::order_by(std::forward<T>(alias_column), ascending);
  return *this;
}

template <typename... Columns>
select_template_t<Columns...> select_template_t<Columns...>::limit(std::size_t count) {
  select_t::limit(count);
  return *this;
}

template <typename... Columns>
select_template_t<Columns...> select_template_t<Columns...>::offset(std::size_t count) {
  select_t::offset(count);
  return *this;
}
template <typename... Columns>
template <typename... TableColumns>
select_template_t<Columns...> select_template_t<Columns...>::group_by(auto TableColumns::*... in_columns) {
  select_t::group_by(in_columns...);
  return *this;
}

template <typename... Columns>
select_template_t<Columns...>::result_type_t<Columns...> select_template_t<Columns...>::operator()() {
  run();
  return result_type_t<Columns...>{*this};
}

template <typename... TableColumns>
typename select_t::result_type_iterator<TableColumns...>::type
select_t::result_type_iterator<TableColumns...>::get() const {
  type result{};
  std::int32_t l_column_index = 0;
  std::int32_t l_tuple_index  = 0;
  const auto l_max_column     = select_.impl_->stmt_->get_column_count();
  constexpr auto l_num_result = std::tuple_size_v<std::tuple<TableColumns...>>;
  const auto l_range_count    = select_.impl_->column_index_ranges_.size();
  const auto l_name_count     = select_.impl_->column_names_.size();

  if (l_range_count != l_num_result) {
    throw std::runtime_error(fmt::format(
        "select result mapping mismatch: range_count={} result_count={}", l_range_count, l_num_result
    ));
  }

  std::size_t l_expected_columns = 0;
  for (const auto& [l_begin, l_end] : select_.impl_->column_index_ranges_) {
    if (l_begin > l_end || l_end > l_name_count) {
      throw std::runtime_error(fmt::format(
          "select column range out of bounds: [{}, {}) with column_names_size={}", l_begin, l_end, l_name_count
      ));
    }
    l_expected_columns += l_end - l_begin;
  }
  if (l_expected_columns != static_cast<std::size_t>(l_max_column)) {
    throw std::runtime_error(fmt::format(
        "select sqlite column count mismatch: expected={} actual={}", l_expected_columns, l_max_column
    ));
  }

  // 生成一个编译期的bool数组，表示每个TableColumn是否是object<Table>
  auto l_iter_fun             = [this, &l_tuple_index, &l_column_index](auto&& in_column) {
    if (l_tuple_index >= static_cast<std::int32_t>(select_.impl_->column_index_ranges_.size())) {
      throw std::runtime_error(fmt::format(
          "select tuple index out of bounds: tuple_index={} ranges_size={}",
          l_tuple_index,
          select_.impl_->column_index_ranges_.size()
      ));
    }

    auto l_range       = select_.impl_->column_index_ranges_[l_tuple_index];
    bool is_value_type = l_range.second == l_range.first + 1;
    // 多列，说明是一个object<Table>，需要从多列中构造出一个Table对象
    for (std::size_t i = l_range.first; i < l_range.second; ++i) {
      if (i >= select_.impl_->column_names_.size()) {
        throw std::runtime_error(fmt::format(
            "select column index out of bounds: column_index={} column_names_size={}",
            i,
            select_.impl_->column_names_.size()
        ));
      }
      if (l_column_index >= select_.impl_->stmt_->get_column_count()) {
        throw std::runtime_error(fmt::format(
            "sqlite stmt column index out of bounds: stmt_column_index={} stmt_column_count={}",
            l_column_index,
            select_.impl_->stmt_->get_column_count()
        ));
      }

      if (is_value_type)
        select_.impl_->column_names_[i]->set_value(*select_.impl_->stmt_, l_column_index, &in_column);
      else
        select_.impl_->column_names_[i]->set_struct_value(*select_.impl_->stmt_, l_column_index, &in_column);
      l_column_index++;
    }

    l_tuple_index++;
  };
  if constexpr (l_num_result == 1) {
    l_iter_fun(result);
  } else {
    std::apply([&](auto&&... column) { (l_iter_fun(column), ...); }, result);
  }

  return result;
}

template <typename T>
select_t select_t::where(T&& condition_fun) {
  auto l_condition_fun_ptr = std::make_shared<std::decay_t<T>>(std::forward<T>(condition_fun));
  impl_->wheres_           = l_condition_fun_ptr;
  return *this;
}

// 获取列信息
template <typename Table, typename Value>
void get_column_info(
    const storage& s, Value Table::* column_ptr, std::vector<std::shared_ptr<base_column_info_t>>& column_infos
)
  requires std::is_member_pointer_v<decltype(column_ptr)>
{
  column_infos.push_back(std::make_shared<column_info_t>(column_ptr));
}
template <typename T>
void get_column_info(const storage& s, T&& alias_column, std::vector<std::shared_ptr<base_column_info_t>>& column_infos)
  requires is_object_specialization_v<std::decay_t<T>>
{
  using Table = class_type_t<std::decay_t<T>>;
  for (const auto& table_column : s.get_table_columns<Table>())
    column_infos.push_back(std::make_shared<column_info_t>(table_column.ptr_));
}
template <typename T>
void get_column_info(const storage& s, T&& alias_column, std::vector<std::shared_ptr<base_column_info_t>>& column_infos)
  requires is_alias_column_t_v<std::decay_t<T>>
{
  column_infos.push_back(std::make_shared<alias_column_info_t>(std::forward<T>(alias_column)));
}
template <typename T>
void get_column_info(const storage& s, T&& alias_column, std::vector<std::shared_ptr<base_column_info_t>>& column_infos)
  requires is_count_t_v<std::decay_t<T>>
{
  column_infos.push_back(std::make_shared<count_column_info_t>(std::forward<T>(alias_column)));
}
template <typename T>
void get_column_info(const storage& s, T&& alias_column, std::vector<std::shared_ptr<base_column_info_t>>& column_infos)
  requires(is_alias_t_v<std::decay_t<T>>)

{
  using Table = class_type_t<std::decay_t<T>>;
  for (const auto& table_column : s.get_table_columns<Table>())
    column_infos.push_back(std::make_shared<alias_column_info_t>(table_column.ptr_, alias_column.table_name_));
}

template <typename... TableColumns>
select_template_t<TableColumns...> select_t::columns(TableColumns... in_columns) {
  std::size_t l_column_index = 0;
  auto l_iter_fun            = [this, &l_column_index](auto&& in_column) {
    auto l_begin = impl_->column_names_.size();
    get_column_info(*impl_->s_, std::forward<decltype(in_column)>(in_column), impl_->column_names_);
    auto l_end = impl_->column_names_.size();
    impl_->column_index_ranges_.emplace_back(l_begin, l_end);
  };
  (l_iter_fun(in_columns), ...);
  return select_template_t<TableColumns...>{std::move(*this)};
}

}  // namespace doodle::orm