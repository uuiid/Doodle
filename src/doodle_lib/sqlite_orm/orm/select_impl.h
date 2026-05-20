#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/alias.h>
#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/count.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <memory>
#include <utility>

namespace doodle::orm {
// select_t 模板方法实现

template <typename FromTable>
select_t select_t::from() {
  impl_->from_table_name_ = impl_->s_->get_table_name<FromTable>();
  return *this;
}

template <typename FromTable>
select_t select_t::join(auto in_ptr, auto in_ref_ptr, join_type in_join_type)
  requires((std::is_member_pointer_v<decltype(in_ptr)>) && (std::is_member_pointer_v<decltype(in_ref_ptr)>))
{
  join_info_t join_info{};
  join_info.type_             = in_join_type;
  join_info.join_table_info_  = std::make_shared<table_info_t>(typeid(FromTable));
  join_info.self_column_info_ = std::make_shared<column_info_t>(in_ptr);
  join_info.join_column_info_ = std::make_shared<column_info_t>(in_ref_ptr);
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
  auto l_create_column_info_ptr = [](auto&& column) -> column_info_ptr {
    if constexpr (is_alias_column_t_v<std::decay_t<decltype(column)>>) {
      return std::make_shared<alias_column_info_t>(std::forward<decltype(column)>(column));
    } else {
      return std::make_shared<column_info_t>(std::forward<decltype(column)>(column));
    }
  };

  join_info_t join_info{};
  join_info.type_             = in_join_type;
  join_info.join_table_info_  = std::make_shared<alias_info_t>(std::forward<JoinTable>(join_table));
  join_info.self_column_info_ = l_create_column_info_ptr(in_ptr);
  join_info.join_column_info_ = l_create_column_info_ptr(in_ref_ptr);
  impl_->joins_.push_back(std::move(join_info));
  return *this;
}

template <typename T>
select_t select_t::order_by(auto T::* in_column_fun, bool ascending) {
  impl_->order_bys_.push_back(impl_->s_->get_column_name(in_column_fun) + (ascending ? "" : " DESC"));
  return *this;
}
template <typename... TableColumns>
select_t select_t::group_by(auto TableColumns::*... in_columns) {
  impl_->group_bys_ = {std::make_shared<column_info_t>(in_columns)...};
  return *this;
}

// result_type_iterator 模板方法实现

template <typename... TableColumns>
select_t::result_type_iterator<TableColumns...>::result_type_iterator(select_t& in_select)
    : select_(&in_select), is_end_(false), cache_(std::make_shared<value_type>()) {
  next();
}

template <typename... TableColumns>
void select_t::result_type_iterator<TableColumns...>::next() {
  if (is_end_ || !select_->impl_->stmt_) return is_end_ = true, void();

  const auto l_rc = select_->impl_->stmt_->step_not_throw();
  if (l_rc == SQLITE_ROW) return is_end_ = false, void();
  if (l_rc == SQLITE_DONE) return is_end_ = true, void();

  is_end_ = true;
  DOODLE_ORM_ERROR_SQLITE3(l_rc, select_->impl_->stmt_->db_);
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
  return select_ == rhs.select_ && is_end_ == rhs.is_end_;
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
  if (l_iter != end()) throw std::runtime_error("More than one row returned");
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
  const auto l_max_column     = select_->impl_->stmt_->get_column_count();
  constexpr auto l_num_result = std::tuple_size_v<std::tuple<TableColumns...>>;
  // 生成一个编译期的bool数组，表示每个TableColumn是否是object<Table>
  std::array<bool, l_num_result> is_object_array{is_object_specialization_v<std::decay_t<TableColumns>>...};
  auto l_iter_fun = [this, &l_column_index, &l_tuple_index, &is_object_array](auto&& in_column) {
    // select_->column_names_[l_column_index]->set_value(*select_->stmt_, l_column_index, &in_column);
    using column_or_struct_type = std::decay_t<decltype(in_column)>;
    if (!is_object_array[l_tuple_index]) {
      select_->impl_->column_names_[l_column_index]->set_value(*select_->impl_->stmt_, l_column_index, &in_column);
      l_column_index++;
    } else /* if constexpr (is_object_specialization_v<column_or_struct_type>) */ {
      for (auto&& table_column_ptr : select_->impl_->s_->get_table_columns<column_or_struct_type>())
        select_->impl_->column_names_[l_column_index]->set_struct_value(
            *select_->impl_->stmt_, l_column_index, &in_column
        ),
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
  auto l_condition_fun_ptr = std::make_shared<T>(std::forward<T>(condition_fun));
  impl_->wheres_           = l_condition_fun_ptr;
  return *this;
}

template <typename... TableColumns>
select_template_t<TableColumns...> select_t::columns(TableColumns... in_columns) {
  auto l_iter_fun = [this](auto&& in_column) {
    // 处理每个参数
    // 如果是成员指针，获取列名
    using column_type = std::decay_t<decltype(in_column)>;
    if constexpr (std::is_member_pointer_v<column_type>) {
      impl_->column_names_.push_back(std::make_shared<column_info_t>(in_column));
    } else if constexpr (is_object_specialization_v<column_type>) {
      // 如果是object<Table>，获取表的所有列名
      using table_type = class_type_t<column_type>;
      for (const auto& table_column : impl_->s_->get_table_columns<table_type>())
        impl_->column_names_.push_back(std::make_shared<column_info_t>(table_column.ptr_));
    } else if constexpr (is_alias_column_t_v<column_type>) {
      impl_->column_names_.push_back(std::make_shared<alias_column_info_t>(in_column));
    } else if constexpr (is_count_t_v<column_type>) {
      impl_->column_names_.push_back(std::make_shared<count_column_info_t>(std::move(in_column)));
    }

    else {
      static_assert(always_false<column_type>, "不支持的参数类型");
    }
  };
  (l_iter_fun(in_columns), ...);
  return select_template_t<TableColumns...>{std::move(*this)};
}

}  // namespace doodle::orm