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
      column_names_.push_back(std::make_shared<column_info_t<class_type_t<column_type>>>(in_column));
    } else if constexpr (is_object_specialization_v<column_type>) {
      // 如果是object<Table>，获取表的所有列名
      using table_type = class_type_t<column_type>;
      for (auto& column_name : s_->get_table_column_names<table_type>()) {
        column_names_.push_back(std::make_shared<column_info_t<table_type>>(column_name));
      }
    } else if constexpr (is_alias_column_info_specialization_v<column_type>) {
      column_names_.push_back(
          std::make_shared<alias_column_info_t<class_type_t<column_type>, class_attr_type_t<column_type>>>(in_column)
      );
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
  const auto l_max_column     = stmt_->get_column_count();
  auto l_iter_fun             = [this, &l_column_index](auto&& in_column) {
    using column_or_struct_type = std::decay_t<decltype(in_column)>;
    if constexpr ((std::is_member_pointer_v<column_type> || is_alias_column_info_specialization_v<column_type>) &&
                  is_in_tuple_v<column_or_struct_type, storage_column_types>) {
      in_column = stmt_->get_column_value<column_or_struct_type>(l_column_index++);
    } else if constexpr (is_object_specialization_v<column_or_struct_type>) {
      for (auto&& table_column_ptr : s_->get_table_columns<column_or_struct_type>()) {
        std::visit(
            [&](auto&& column_ptr) {
              using column_type     = class_attr_type_t<std::decay_t<decltype(column_ptr)>>;
              in_column.*column_ptr = stmt_->get_column_value<column_type>(l_column_index++);
            },
            table_column_ptr.ptr_
        );
      }
    } else {
      static_assert(always_false<column_or_struct_type>, "不支持的参数类型");
    }
  };

  std::apply([&](auto&&... column) { (l_iter_fun(column), ...); }, result);

  return result;
}

}  // namespace doodle::orm