#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {
template <typename T>
  requires(is_column_operations_specialization_v<T>)
select_t& select_t::where(T&& condition_fun) {
  auto l_condition_fun_ptr = std::make_shared<T>(std::forward<T>(condition_fun));
  wheres_                  = l_condition_fun_ptr;
  return *this;
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
            table_column_ptr.ptr_.ptr_
        );
      }
    }
  };

  std::apply([&](auto&&... column) { (l_iter_fun(column), ...); }, result);

  return result;
}

template <typename... TableColumns>
select_result_type<TableColumns...>& select_result_type<TableColumns...>::operator()() & {
  if (!stmt_) {
    auto l_sql = select_t::to_sql(*s_);
    stmt_      = std::make_shared<sqlite_stmt>();
    stmt_->prepare(*s_, l_sql);
    wheres_->collect_bind_variants(bind_variants_);
  }
  stmt_->reset_bind_index();
  for (const auto& val : bind_variants_) {
    stmt_->bind(*val);
  }
  return *this;
}

template <typename... TableColumns>
select_result_type<TableColumns...> select_result_type<TableColumns...>::operator()() && {
  (void)operator()();
  return std::move(*this);
}

}  // namespace doodle::orm