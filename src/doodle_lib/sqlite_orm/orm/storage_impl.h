#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/select.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <atomic>
#include <map>
#include <memory>
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <variant>
#include <vector>

namespace doodle::orm {

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
std::string storage::get_column_name(auto T::* in_ptr, bool add_table_name) const {
  auto l_type_index = std::type_index(typeid(T));
  if (!type_to_table_index_.contains(l_type_index)) throw std::runtime_error("Table not found for the given type");

  auto l_table_index = type_to_table_index_.at(l_type_index);
  auto& l_table      = static_cast<table_info<T>&>(*tables_[l_table_index]);
  auto& l_column     = l_table.find_column_info(in_ptr);
  return add_table_name ? fmt::format(R"("{}"."{}")", l_table.name_, l_column.ptr_.name_) : l_column.ptr_.name_;
}
template <typename T>
std::string storage::get_column_name(const table_columns_t<T>& in_column, bool add_table_name) const {
  auto l_type_index = std::type_index(typeid(T));
  if (!type_to_table_index_.contains(l_type_index)) throw std::runtime_error("Table not found for the given type");

  auto l_table_index = type_to_table_index_.at(l_type_index);
  auto& l_table      = static_cast<table_info<T>&>(*tables_[l_table_index]);
  auto& l_column     = l_table.find_column_info(in_column);
  return add_table_name ? fmt::format(R"("{}"."{}")", l_table.name_, l_column.ptr_.name_) : l_column.ptr_.name_;
}

template <typename T>
std::vector<std::string> storage::get_table_column_names() const {
  auto l_type_index = std::type_index(typeid(T));
  if (!type_to_table_index_.contains(l_type_index)) throw std::runtime_error("Table not found for the given type");
  auto l_table_index = type_to_table_index_.at(l_type_index);
  auto& l_table      = static_cast<table_info<T>&>(*tables_[l_table_index]);
  std::vector<std::string> column_names;
  for (const auto& column : l_table.columns_) {
    column_names.push_back(fmt::format(R"("{}"."{}")", l_table.name_, column.ptr_.name_));
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
  l_fk.ptr_       = l_self_table.find_column_info(in_ptr).ptr_.name_;
  l_fk.ref_table_ = l_ref_table.name_;
  l_fk.ref_ptr_   = l_ref_table.find_column_info(in_ref_ptr).ptr_.name_;
  l_fk.on_delete_ = on_delete;
  l_fk.on_update_ = on_update;
  l_self_table.foreign_keys_.push_back(std::move(l_fk));
}
template <typename T>
void storage::reg_index(std::string&& in_name, auto T::* in_ptr) {
  auto l_type_index = std::type_index(typeid(T));
  if (!type_to_table_index_.contains(l_type_index)) throw std::runtime_error("Table not found for the given type");
  auto l_table_index = type_to_table_index_.at(l_type_index);
  auto& l_table      = static_cast<table_info<T>&>(*tables_[l_table_index]);
  index_info l_index{};
  l_index.name_ = std::move(in_name);
  l_index.ptr_  = l_table.find_column_info(in_ptr).ptr_.name_;
  indexes_.push_back(std::move(l_index));
}
template <typename T>
void storage::reg_unique_index(std::string&& in_name, auto... in_ptrs) {
  auto l_type_index = std::type_index(typeid(T));
  if (!type_to_table_index_.contains(l_type_index)) throw std::runtime_error("Table not found for the given type");
  auto l_table_index = type_to_table_index_.at(l_type_index);
  auto& l_table      = static_cast<table_info<T>&>(*tables_[l_table_index]);
  unique_index_info l_unique_index{};
  l_unique_index.name_ = std::move(in_name);
  ((l_unique_index.ptrs_.push_back(
       unique_index_info::table_and_column{l_table.name_, l_table.find_column_info(in_ptrs).ptr_.name_}
   )),
   ...);
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