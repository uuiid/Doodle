#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {

template <typename... TableColumns>
typename select_result_type_iterator<TableColumns...>::type select_result_type_iterator<TableColumns...>::get() const {
  type result{};
  std::int32_t l_column_index = 0;
  const auto l_max_column     = stmt_->get_column_count();
  auto l_iter_fun             = [this, &l_column_index](auto&& in_column) {
    using column_or_struct_type = std::decay_t<decltype(in_column)>;
    if constexpr (is_in_tuple_v<column_or_struct_type, storage_column_types>) {
      sqlite_statement_extractor<column_or_struct_type> l_extractor{};
      in_column = l_extractor.extract(stmt_->stmt_, l_column_index++);
    } else {
      for (auto&& table_column_ptr : s_->get_table_columns<column_or_struct_type>()) {
        std::visit(
            [&](auto&& column_ptr) {
              using column_type = member_type_t<std::decay_t<decltype(column_ptr)>>;
              sqlite_statement_extractor<column_type> l_extractor{};
              in_column.*column_ptr = l_extractor.extract(stmt_->stmt_, l_column_index++);
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
select_result_type<TableColumns...>& select_result_type<TableColumns...>::operator()(storage& s) {
  auto l_sql      = select_t::to_sql(s);
  auto l_stmt_ptr = std::make_shared<sqlite_stmt>();
  l_stmt_ptr->prepare(s, l_sql);
  wheres_.bind(*l_stmt_ptr);

  s_    = &s;
  stmt_ = std::move(l_stmt_ptr);
  return *this;
}

}  // namespace doodle::orm