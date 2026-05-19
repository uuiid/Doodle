#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/sqlite_statement.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include "fwd.h"
#include <algorithm>
#include <any>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>

namespace doodle::orm {
template <typename T>
bind_value_collector_t::bind_value_t::bind_value_t(T&& value) {
  // 如果是 char* 或 const char*，需要转换为 std::string 存储在 variant 中，否则会有生命周期问题
  if constexpr (std::is_convertible_v<T, std::string>) {
    value_    = std::string(std::forward<T>(value));
    bind_fun_ = [](const bind_value_collector_t::bind_value_t& self, sqlite_stmt& stmt) {
      using actual_type = std::string;
      auto& str_value   = std::any_cast<const actual_type&>(self.value_);
      stmt.bind<actual_type>(str_value);
    };
  } else {
    using value_type = std::decay_t<T>;
    value_           = std::forward<T>(value);
    bind_fun_        = [](const bind_value_collector_t::bind_value_t& self, sqlite_stmt& stmt) {
      using actual_type = value_type;
      auto& value_ref   = std::any_cast<const actual_type&>(self.value_);
      stmt.bind<actual_type>(value_ref);
    };
  }
}

template <typename T>
column_info& table_info_base::find_column_info(auto T::* in_ptr) {
  auto l_iter = std::find_if(columns_.begin(), columns_.end(), [in_ptr](const column_info& in_column) {
    return in_column.ptr_.is(in_ptr);
  });
  if (l_iter == columns_.end()) throw std::runtime_error("Column not found for the given member pointer");

  return *l_iter;
}

template <typename T>
T sqlite_stmt::get_column_value(int columnIndex) const {
  sqlite_statement_extractor<T> l_extractor{};
  return l_extractor.extract(stmt_, columnIndex);
}
template <typename T>
void sqlite_stmt::bind(const T& in_value) {
  sqlite_statement_binder<T> l_binder{};
  auto l_rt = l_binder.bind(stmt_, get_bind_index(), in_value);
  DOODLE_ORM_ERROR_SQLITE3(l_rt, db_);
}

template <typename T>
table_fts_info& table_fts_info::add_column(std::string&& in_name, auto T::* in_ptr, auto... in_options) {
  column_info l_column;
  l_column.name_    = std::move(in_name);
  l_column.ptr_     = table_columns_t{in_ptr};
  bool is_unindexed = false;
  // 解析 in_options
  (([&]() {
     if constexpr (std::is_same_v<decltype(in_options), decltype(not_null())>) {
       l_column.not_null_ = true;
     } else if constexpr (std::is_same_v<decltype(in_options), decltype(primary_key())>) {
       l_column.primary_key_ = true;
     } else if constexpr (std::is_same_v<decltype(in_options), decltype(autoincrement())>) {
       l_column.autoincrement_ = true;
     } else if constexpr (std::is_same_v<decltype(in_options), decltype(unindexed())>) {
       is_unindexed = true;
     } else if constexpr (std::is_same_v<decltype(in_options), decltype(unique())>) {
       l_column.unique_ = true;
     }
   }()),
   ...);
  // 根据成员变量类型推断 column_type
  using member_type = std::remove_reference_t<std::remove_pointer_t<class_attr_type_t<decltype(in_ptr)>>>;
  l_column.type_    = sqlite_statement_printer<member_type>{}();
  columns_.push_back(std::move(l_column));
  unindexed_columns_.push_back(is_unindexed);
  return *this;
}

template <typename T>
table_info& table_info::add_column(std::string&& in_name, auto T::* in_ptr, auto... in_options) {
  column_info l_column;
  l_column.name_ = std::move(in_name);
  l_column.ptr_  = table_columns_t{in_ptr};
  // 解析 in_options
  (([&]() {
     if constexpr (std::is_same_v<decltype(in_options), decltype(not_null())>) {
       l_column.not_null_ = true;
     } else if constexpr (std::is_same_v<decltype(in_options), decltype(primary_key())>) {
       l_column.primary_key_ = true;
     } else if constexpr (std::is_same_v<decltype(in_options), decltype(autoincrement())>) {
       l_column.autoincrement_ = true;
     } else if constexpr (std::is_same_v<decltype(in_options), decltype(unique())>) {
       l_column.unique_ = true;
     } else if constexpr (std::is_same_v<decltype(in_options), decltype(default_value(""))>) {
       l_column.default_value_ = in_options.value_;
     }
   }()),
   ...);
  // 根据成员变量类型推断 column_type
  using member_type = std::remove_reference_t<std::remove_pointer_t<class_attr_type_t<decltype(in_ptr)>>>;
  columns_.push_back(std::move(l_column));
  return *this;
}
template <typename T>
table_info& storage::reg_table(std::string&& in_name) {
  auto l_table                               = std::make_shared<table_info>();
  l_table->name_                             = std::move(in_name);
  l_table->type_index_                       = std::type_index(typeid(T));
  type_to_table_index_[l_table->type_index_] = tables_.size();
  tables_.push_back(std::move(l_table));
  return static_cast<table_info&>(*tables_.back());
}

template <typename T>
std::string storage::get_table_name() const {
  return get_table_name(std::type_index(typeid(T)));
}
template <typename T>
std::string storage::get_column_name(auto T::* in_ptr, bool add_table_name) const {
  auto l_type_index = std::type_index(typeid(T));
  if (!type_to_table_index_.contains(l_type_index)) throw std::runtime_error("Table not found for the given type");

  auto l_table_index = type_to_table_index_.at(l_type_index);
  auto& l_table      = static_cast<table_info&>(*tables_[l_table_index]);
  auto& l_column     = l_table.find_column_info(in_ptr);
  return add_table_name ? fmt::format(R"("{}"."{}")", l_table.name_, l_column.name_) : l_column.name_;
}

std::string storage::get_column_name(const table_columns_t& in_column, bool add_table_name) const {
  auto l_type_index = in_column.table_type_index_;
  if (!type_to_table_index_.contains(l_type_index)) throw std::runtime_error("Table not found for the given type");

  auto l_table_index = type_to_table_index_.at(l_type_index);
  auto& l_table      = static_cast<table_info&>(*tables_[l_table_index]);
  auto& l_column     = l_table.find_column_info(in_column);
  return add_table_name ? fmt::format(R"("{}"."{}")", l_table.name_, l_column.name_) : l_column.name_;
}

template <typename T>
std::vector<std::string> storage::get_table_column_names() const {
  auto l_type_index = std::type_index(typeid(T));
  if (!type_to_table_index_.contains(l_type_index)) throw std::runtime_error("Table not found for the given type");
  auto l_table_index = type_to_table_index_.at(l_type_index);
  auto& l_table      = *tables_[l_table_index];
  std::vector<std::string> column_names;
  for (const auto& column : l_table.columns_) {
    column_names.push_back(fmt::format(R"("{}"."{}")", l_table.name_, column.name_));
  }
  return column_names;
}

template <typename T>
bool storage::has_reg_table() {
  return type_to_table_index_.contains(std::type_index(typeid(T)));
}

template <typename T>
const std::vector<column_info>& storage::get_table_columns() const {
  auto l_type_index = std::type_index(typeid(T));
  if (!type_to_table_index_.contains(l_type_index)) throw std::runtime_error("Table not found for the given type");
  auto l_table_index = type_to_table_index_.at(l_type_index);
  auto& l_table      = *tables_[l_table_index];
  return l_table.columns_;
}

template <typename T, typename T2>
void storage::reg_foreign_key(
    std::string&& in_name, auto T::* in_ptr, auto T2::* in_ref_ptr, foreign_key_action on_delete,
    foreign_key_action on_update
) {
  auto l_self_type_index = std::type_index(typeid(T));
  auto l_ref_type_index  = std::type_index(typeid(T2));
  if (!type_to_table_index_.contains(l_self_type_index)) throw std::runtime_error("Table not found for the given type");
  if (!type_to_table_index_.contains(l_ref_type_index)) throw std::runtime_error("Table not found for the given type");
  auto l_self_table_index = type_to_table_index_.at(l_self_type_index);
  auto l_ref_table_index  = type_to_table_index_.at(l_ref_type_index);
  auto& l_self_table      = *tables_[l_self_table_index];
  auto& l_ref_table       = *tables_[l_ref_table_index];
  foreign_key_info l_fk{};
  l_fk.name_      = std::move(in_name);
  l_fk.ptr_       = l_self_table.find_column_info(in_ptr).name_;
  l_fk.ref_table_ = l_ref_table.name_;
  l_fk.ref_ptr_   = l_ref_table.find_column_info(in_ref_ptr).name_;
  l_fk.on_delete_ = on_delete;
  l_fk.on_update_ = on_update;
  l_self_table.foreign_keys_.push_back(std::move(l_fk));
  // 生成索引以优化外键约束的性能
  reg_index<T>(fmt::format("idx_{}_{}", l_self_table.name_, l_fk.ptr_), in_ptr);
  reg_index<T2>(fmt::format("idx_{}_{}", l_ref_table.name_, l_fk.ref_ptr_), in_ref_ptr);
}
template <typename T>
void storage::reg_index(std::string&& in_name, auto T::* in_ptr) {
  auto l_type_index = std::type_index(typeid(T));
  if (!type_to_table_index_.contains(l_type_index)) throw std::runtime_error("Table not found for the given type");
  auto l_table_index = type_to_table_index_.at(l_type_index);
  auto& l_table      = *tables_[l_table_index];
  index_info l_index{};
  l_index.name_        = std::move(in_name);
  l_index.table_name_  = l_table.name_;
  l_index.column_name_ = l_table.find_column_info(in_ptr).name_;
  if (std::ranges::find_if(indexes_, [&](const index_info& in_index) {
        return in_index.table_name_ == l_index.table_name_ && in_index.column_name_ == l_index.column_name_;
      }) != indexes_.end()) {
    SPDLOG_WARN("Index on {}.{} already exists, skipping index creation", l_table.name_, l_index.column_name_);
    return;  // 已经存在相同的索引，无需重复创建
  }
  indexes_.push_back(std::move(l_index));
}
template <typename T>
void storage::reg_unique_index(std::string&& in_name, auto... in_ptrs) {
  auto l_type_index = std::type_index(typeid(T));
  if (!type_to_table_index_.contains(l_type_index)) throw std::runtime_error("Table not found for the given type");
  auto l_table_index = type_to_table_index_.at(l_type_index);
  auto& l_table      = *tables_[l_table_index];
  unique_index_info l_unique_index{};
  l_unique_index.name_       = std::move(in_name);
  l_unique_index.table_name_ = l_table.name_;
  ((l_unique_index.ptrs_.push_back(l_table.find_column_info(in_ptrs).name_)), ...);
  if (std::ranges::find_if(unique_indexes_, [&](const unique_index_info& in_index) {
        return in_index.table_name_ == l_unique_index.table_name_ && in_index.ptrs_ == l_unique_index.ptrs_;
      }) != unique_indexes_.end()) {
    SPDLOG_WARN(
        "Unique index on {}.{} already exists, skipping index creation", l_table.name_,
        fmt::join(l_unique_index.ptrs_, ", ")
    );
    return;  // 已经存在相同的唯一索引，无需重复创建
  }
  unique_indexes_.push_back(std::move(l_unique_index));
}

template <typename T, typename RefTable>
table_info& table_info::add_foreign_key(
    std::string&& in_name, auto T::* in_ptr, auto RefTable::* in_ref_ptr, foreign_key_action on_delete,
    foreign_key_action on_update
) {
  to_register_.push_back([name_ = std::move(in_name), in_ptr, in_ref_ptr, on_delete, on_update](storage& s) mutable {
    s.reg_foreign_key<T, RefTable>(std::move(name_), in_ptr, in_ref_ptr, on_delete, on_update);
  });
  return *this;
}
template <typename T>
table_info& table_info::add_index(std::string&& in_name, auto T::* in_ptr) {
  to_register_.push_back([name_ = std::move(in_name), in_ptr](storage& s) mutable {
    s.reg_index<T>(std::move(name_), in_ptr);
  });
  return *this;
}
template <typename T>
table_info& table_info::add_unique_index(std::string&& in_name, auto T::*... in_ptrs) {
  to_register_.push_back([name_ = std::move(in_name), in_ptrs...](storage& s) mutable {
    s.reg_unique_index<T>(std::move(name_), in_ptrs...);
  });
  return *this;
}

}  // namespace doodle::orm