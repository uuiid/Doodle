#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/sqlite_statement.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <typeindex>
#include <utility>
#include <vector>

namespace doodle::orm {

template <typename T>
std::string table_info_t<T>::get_table_name(const storage& s) const {
  return s.get_table_name<T>();
}

template <typename T>
T sqlite_stmt::get_column_value(int columnIndex) const {
  sqlite_statement_extractor<T> l_extractor{};
  return l_extractor.extract(stmt_, columnIndex);
}

template <typename T>
table_fts_info<T>& table_fts_info<T>::content(const std::string& content_table, const std::string& content_rowid) {
  content_table_ = content_table;
  content_rowid_ = content_rowid;
  return *this;
}

template <typename T>
table_fts_info<T>& table_fts_info<T>::tokenizer(const std::string& tokenizer) {
  tokenizer_ = tokenizer;
  return *this;
}

template <typename T>
typename table_fts_info<T>::column_info_fts_t& table_fts_info<T>::find_column_info(auto T::* in_ptr) {
  auto l_iter = std::find_if(columns_.begin(), columns_.end(), [in_ptr](const column_info_fts_t& in_column) {
    return in_column.ptr_ == column_ptr_type{in_ptr};
  });
  if (l_iter == columns_.end()) throw std::runtime_error("Column not found for the given member pointer");

  return *l_iter;
}

template <typename T>
typename table_fts_info<T>::column_info_fts_t& table_fts_info<T>::find_column_info(
    const table_columns_t<T>& in_column
) {
  auto l_iter = std::find_if(columns_.begin(), columns_.end(), [&in_column](const column_info_fts_t& in_col) {
    return in_col.ptr_ == in_column;
  });
  if (l_iter == columns_.end()) throw std::runtime_error("Column not found for the given column pointer");

  return *l_iter;
}

template <typename T>
table_fts_info<T>& table_fts_info<T>::add_column(std::string&& in_name, auto T::* in_ptr, auto... in_options) {
  column_info_fts_t l_column;
  l_column.name_ = std::move(in_name);
  l_column.ptr_  = in_ptr;
  // 解析 in_options
  (([&]() {
     if constexpr (std::is_same_v<decltype(in_options), decltype(not_null())>) {
       l_column.not_null_ = true;
     } else if constexpr (std::is_same_v<decltype(in_options), decltype(primary_key())>) {
       l_column.primary_key_ = true;
     } else if constexpr (std::is_same_v<decltype(in_options), decltype(autoincrement())>) {
       l_column.autoincrement_ = true;
     }
   }()),
   ...);
  // 根据成员变量类型推断 column_type
  using member_type = std::remove_reference_t<std::remove_pointer_t<class_attr_type_t<decltype(in_ptr)>>>;
  l_column.type_    = sqlite_statement_printer<member_type>{}();
  columns_.push_back(std::move(l_column));
  return *this;
}

template <typename T>
std::string table_fts_info<T>::get_table_create_sql() const {
  std::vector<std::string> l_column_sqls;
  for (const auto& column : columns_) {
    std::string l_sql = fmt::format("{} {}", column.name_, column.type_);
    if (column.unindexed_) {
      l_sql += " UNINDEXED";
    }
    l_column_sqls.push_back(std::move(l_sql));
  }
  if (!content_table_.empty()) l_column_sqls.push_back(fmt::format("CONTENT={}", content_table_));

  if (!content_rowid_.empty()) l_column_sqls.push_back(fmt::format("CONTENT_ROWID={}", content_rowid_));

  if (!tokenizer_.empty()) l_column_sqls.push_back(fmt::format("TOKENIZER={}", tokenizer_));
  return fmt::format("CREATE VIRTUAL TABLE IF NOT EXISTS {} USING fts5 ({})", name_, fmt::join(l_column_sqls, ", "));
}

template <typename T>
column_info<T>& table_info<T>::find_column_info(auto T::* in_ptr) {
  auto l_iter = std::find_if(columns_.begin(), columns_.end(), [in_ptr](const column_info<T>& in_column) {
    return in_column.ptr_ == column_ptr_type{in_ptr};
  });
  if (l_iter == columns_.end()) throw std::runtime_error("Column not found for the given member pointer");

  return *l_iter;
}

template <typename T>
column_info<T>& table_info<T>::find_column_info(const table_columns_t<T>& in_column) {
  auto l_iter = std::find_if(columns_.begin(), columns_.end(), [&in_column](const column_info<T>& in_col) {
    return in_col.ptr_ == in_column;
  });
  if (l_iter == columns_.end()) throw std::runtime_error("Column not found for the given column pointer");

  return *l_iter;
}

template <typename T>
table_info<T>& table_info<T>::add_column(std::string&& in_name, auto T::* in_ptr, auto... in_options) {
  column_info<T> l_column;
  l_column.name_ = std::move(in_name);
  l_column.ptr_  = in_ptr;
  // 解析 in_options
  (([&]() {
     if constexpr (std::is_same_v<decltype(in_options), decltype(not_null())>) {
       l_column.not_null_ = true;
     } else if constexpr (std::is_same_v<decltype(in_options), decltype(primary_key())>) {
       l_column.primary_key_ = true;
     } else if constexpr (std::is_same_v<decltype(in_options), decltype(autoincrement())>) {
       l_column.autoincrement_ = true;
     } else if constexpr (std::is_same_v<decltype(in_options), decltype(unindexed())>) {
       l_column.unindexed_ = true;
     }
   }()),
   ...);
  // 根据成员变量类型推断 column_type
  using member_type = std::remove_reference_t<std::remove_pointer_t<class_attr_type_t<decltype(in_ptr)>>>;
  l_column.type_    = sqlite_statement_printer<member_type>{}();
  columns_.push_back(std::move(l_column));
  return *this;
}

template <typename T>
std::string table_info<T>::get_table_create_sql() const {
  std::vector<std::string> l_column_sqls;
  for (const auto& column : columns_) {
    std::string l_sql = fmt::format("{} {}", column.name_, column.type_);
    if (column.primary_key_) {
      l_sql += " PRIMARY KEY";
    }
    if (column.autoincrement_) {
      l_sql += " AUTOINCREMENT";
    }
    if (column.not_null_) {
      l_sql += " NOT NULL";
    }
    if (column.unique_) {
      l_sql += " UNIQUE";
    }
    l_column_sqls.push_back(std::move(l_sql));
  }
  auto l_fk_sqls = get_foreign_key_create_sql();
  l_column_sqls.insert(l_column_sqls.end(), l_fk_sqls.begin(), l_fk_sqls.end());
  return fmt::format("CREATE TABLE IF NOT EXISTS {} ({})", name_, fmt::join(l_column_sqls, ", "));
}

template <typename T>
table_info<T>& storage::reg_table(std::string&& in_name) {
  auto l_table                               = std::make_shared<table_info<T>>();
  l_table->name_                             = std::move(in_name);
  l_table->type_index_                       = std::type_index(typeid(T));
  type_to_table_index_[l_table->type_index_] = tables_.size();
  tables_.push_back(std::move(l_table));
  return static_cast<table_info<T>&>(*tables_.back());
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
  auto& l_table      = static_cast<table_info<T>&>(*tables_[l_table_index]);
  auto& l_column     = l_table.find_column_info(in_ptr);
  return add_table_name ? fmt::format(R"("{}"."{}")", l_table.name_, l_column.name_) : l_column.name_;
}
template <typename T>
std::string storage::get_column_name(const table_columns_t<T>& in_column, bool add_table_name) const {
  auto l_type_index = std::type_index(typeid(T));
  if (!type_to_table_index_.contains(l_type_index)) throw std::runtime_error("Table not found for the given type");

  auto l_table_index = type_to_table_index_.at(l_type_index);
  auto& l_table      = static_cast<table_info<T>&>(*tables_[l_table_index]);
  auto& l_column     = l_table.find_column_info(in_column);
  return add_table_name ? fmt::format(R"("{}"."{}")", l_table.name_, l_column.name_) : l_column.name_;
}

template <typename T>
std::vector<std::string> storage::get_table_column_names() const {
  auto l_type_index = std::type_index(typeid(T));
  if (!type_to_table_index_.contains(l_type_index)) throw std::runtime_error("Table not found for the given type");
  auto l_table_index = type_to_table_index_.at(l_type_index);
  auto& l_table      = static_cast<table_info<T>&>(*tables_[l_table_index]);
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
const std::vector<column_info<T>>& storage::get_table_columns() const {
  auto l_type_index = std::type_index(typeid(T));
  if (!type_to_table_index_.contains(l_type_index)) throw std::runtime_error("Table not found for the given type");
  auto l_table_index = type_to_table_index_.at(l_type_index);
  auto& l_table      = static_cast<table_info<T>&>(*tables_[l_table_index]);
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
  auto& l_self_table      = static_cast<table_info<T>&>(*tables_[l_self_table_index]);
  auto& l_ref_table       = static_cast<table_info<T2>&>(*tables_[l_ref_table_index]);
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
  auto& l_table      = static_cast<table_info<T>&>(*tables_[l_table_index]);
  index_info l_index{};
  l_index.name_        = std::move(in_name);
  l_index.table_name_  = l_table.name_;
  l_index.column_name_ = l_table.find_column_info(in_ptr).name_;
  indexes_.push_back(std::move(l_index));
}
template <typename T>
void storage::reg_unique_index(std::string&& in_name, auto... in_ptrs) {
  auto l_type_index = std::type_index(typeid(T));
  if (!type_to_table_index_.contains(l_type_index)) throw std::runtime_error("Table not found for the given type");
  auto l_table_index = type_to_table_index_.at(l_type_index);
  auto& l_table      = static_cast<table_info<T>&>(*tables_[l_table_index]);
  unique_index_info l_unique_index{};
  l_unique_index.name_       = std::move(in_name);
  l_unique_index.table_name_ = l_table.name_;
  ((l_unique_index.ptrs_.push_back(l_table.find_column_info(in_ptrs).name_)), ...);
  unique_indexes_.push_back(std::move(l_unique_index));
}

template <typename T>
template <typename RefTable>
table_info<T>& table_info<T>::add_foreign_key(
    std::string&& in_name, auto T::* in_ptr, auto RefTable::* in_ref_ptr, foreign_key_action on_delete,
    foreign_key_action on_update
) {
  to_register_.push_back([name_ = std::move(in_name), in_ptr, in_ref_ptr, on_delete, on_update](storage& s) mutable {
    s.reg_foreign_key<T, RefTable>(std::move(name_), in_ptr, in_ref_ptr, on_delete, on_update);
  });
  return *this;
}
template <typename T>
table_info<T>& table_info<T>::add_index(std::string&& in_name, auto T::* in_ptr) {
  to_register_.push_back([name_ = std::move(in_name), in_ptr](storage& s) mutable {
    s.reg_index<T>(std::move(name_), in_ptr);
  });
  return *this;
}
template <typename T>
table_info<T>& table_info<T>::add_unique_index(std::string&& in_name, auto... in_ptrs) {
  to_register_.push_back([name_ = std::move(in_name), in_ptrs...](storage& s) mutable {
    s.reg_unique_index<T>(std::move(name_), in_ptrs...);
  });
  return *this;
}

}  // namespace doodle::orm