#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {
template <typename T>
select_t& select_t::where(T&& condition_fun) {
  auto l_condition_fun_ptr = std::make_shared<T>(std::forward<T>(condition_fun));
  wheres_                  = l_condition_fun_ptr;
  return *this;
}

template <typename... TableColumns>
typename select_t::result_type_iterator<TableColumns...>::type
select_t::result_type_iterator<TableColumns...>::get() const {
  type result{};
  std::int32_t l_column_index = 0;
  const auto l_max_column     = stmt_->get_column_count();
  auto l_iter_fun             = [this, &l_column_index](auto&& in_column) {
    using column_or_struct_type = std::decay_t<decltype(in_column)>;
    if constexpr (is_in_tuple_v<column_or_struct_type, storage_column_types>) {
      in_column = stmt_->get_column_value<column_or_struct_type>(l_column_index++);
    } else {
      for (auto&& table_column_ptr : s_->get_table_columns<column_or_struct_type>()) {
        std::visit(
            [&](auto&& column_ptr) {
              using column_type     = member_type_t<std::decay_t<decltype(column_ptr)>>;
              in_column.*column_ptr = stmt_->get_column_value<column_type>(l_column_index++);
            },
            table_column_ptr.ptr_
        );
      }
    }
  };

  std::apply([&](auto&&... column) { (l_iter_fun(column), ...); }, result);

  return result;
}

template <typename... TableColumns>
typename select_result_type_iterator<TableColumns...>::type select_result_type_iterator<TableColumns...>::get() const {
  type result{};
  std::int32_t l_column_index = 0;
  const auto l_max_column     = stmt_->get_column_count();
  auto l_iter_fun             = [this, &l_column_index](auto&& in_column) {
    using column_or_struct_type = std::decay_t<decltype(in_column)>;
    if constexpr (is_in_tuple_v<column_or_struct_type, storage_column_types>) {
      in_column = stmt_->get_column_value<column_or_struct_type>(l_column_index++);
    } else {
      for (auto&& table_column_ptr : s_->get_table_columns<column_or_struct_type>()) {
        std::visit(
            [&](auto&& column_ptr) {
              using column_type     = member_type_t<std::decay_t<decltype(column_ptr)>>;
              in_column.*column_ptr = stmt_->get_column_value<column_type>(l_column_index++);
            },
            table_column_ptr.ptr_
        );
      }
    }
  };

  std::apply([&](auto&&... column) { (l_iter_fun(column), ...); }, result);

  return result;
}

template <typename... TableColumns>
select_result_type<TableColumns...>& select_result_type<TableColumns...>::operator()() & {
  run();
  return *this;
}

template <typename... TableColumns>
select_result_type<TableColumns...> select_result_type<TableColumns...>::operator()() && {
  run();
  return std::move(*this);
}

}  // namespace doodle::orm