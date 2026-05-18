#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/alias.h>
#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <memory>

namespace doodle::orm {
template <typename T>
select_t& select_t::where(T&& condition_fun) {
  auto l_condition_fun_ptr = std::make_shared<T>(std::forward<T>(condition_fun));
  wheres_                  = l_condition_fun_ptr;
  return *this;
}
template <typename... TableColumns>
select_t& select_t::columns_(TableColumns... in_columns) {
  auto l_iter_fun = [this](auto&& in_column) {
    // 处理每个参数
    // 如果是成员指针，获取列名
    using column_type = std::decay_t<decltype(in_column)>;
    if constexpr (std::is_member_pointer_v<column_type>) {
      column_names_.push_back(std::make_shared<column_info_t>(in_column));
    } else if constexpr (is_object_specialization_v<column_type>) {
      // 如果是object<Table>，获取表的所有列名
      using table_type = class_type_t<column_type>;
      for (const auto& table_column : s_->get_table_columns<table_type>())
        column_names_.push_back(std::make_shared<column_info_t>(table_column.ptr_));
    } else if constexpr (is_alias_column_t_v<column_type>) {
      column_names_.push_back(std::make_shared<alias_column_info_t>(in_column));
    } else {
      static_assert(always_false<column_type>, "不支持的参数类型");
    }
  };
  (l_iter_fun(in_columns), ...);
  return *this;
};

template <typename... TableColumns>
typename select_t::result_type_iterator<TableColumns...>::type
select_t::result_type_iterator<TableColumns...>::get() const {
  type result{};
  std::int32_t l_column_index = 0;
  std::int32_t l_tuple_index  = 0;
  const auto l_max_column     = select_->stmt_->get_column_count();
  constexpr auto l_num_result = std::tuple_size_v<std::tuple<TableColumns...>>;
  // 生成一个编译期的bool数组，表示每个TableColumn是否是object<Table>
  std::array<bool, l_num_result> is_object_array{is_object_specialization_v<std::decay_t<TableColumns>>...};

  auto l_iter_fun = [this, &l_column_index, &l_tuple_index](auto&& in_column) {
    // select_->column_names_[l_column_index]->set_value(*select_->stmt_, l_column_index, &in_column);
    using column_or_struct_type = std::decay_t<decltype(in_column)>;
    if (!is_object_array[l_tuple_index]) {
      select_->column_names_[l_column_index]->set_value(*select_->stmt_, l_column_index, &in_column);
      l_column_index++;
    } else /* if constexpr (is_object_specialization_v<column_or_struct_type>) */ {
      for (auto&& table_column_ptr : select_->s_->get_table_columns<column_or_struct_type>())
        select_->column_names_[l_column_index]->set_struct_value(*select_->stmt_, l_column_index, &in_column),
            l_column_index++;
    }
    l_tuple_index++;
  };
  std::apply([&](auto&&... column) { (l_iter_fun(column), ...); }, result);

  return result;
}

}  // namespace doodle::orm