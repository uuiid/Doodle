#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
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
std::string storage::get_column_name(auto T::* in_ptr) const {
  auto l_table_index = type_to_table_index_.at(std::type_index(typeid(T)));
  auto& l_table      = static_cast<table_info<T>&>(*tables_[l_table_index]);
  auto& l_column     = l_table.find_column_info(in_ptr);
  return fmt::format(R"("{}"."{}")", l_table.name_, l_column.ptr_.name_);
}
template <typename T>
std::vector<std::string> storage::get_table_column_names() const {
  auto l_table_index = type_to_table_index_.at(std::type_index(typeid(T)));
  auto& l_table      = static_cast<table_info<T>&>(*tables_[l_table_index]);
  std::vector<std::string> column_names;
  for (const auto& column : l_table.columns_) {
    column_names.push_back(fmt::format(R"("{}"."{}")", l_table.name_, column.ptr_.name_));
  }
  return column_names;
}

template <typename T, typename T2>
void storage::reg_foreign_key(
    std::string&& in_name, auto T::* in_ptr, auto T2::* in_ref_ptr, foreign_key_action on_delete,
    foreign_key_action on_update
) {
  auto l_self_table_index = type_to_table_index_[std::type_index(typeid(T))];
  auto l_ref_table_index  = type_to_table_index_[std::type_index(typeid(T2))];
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
  auto l_table_index = type_to_table_index_[std::type_index(typeid(T))];
  auto& l_table      = static_cast<table_info<T>&>(*tables_[l_table_index]);
  index_info l_index{};
  l_index.name_ = std::move(in_name);
  l_index.ptr_  = l_table.find_column_info(in_ptr).ptr_.name_;
  indexes_.push_back(std::move(l_index));
}
template <typename T>
void storage::reg_unique_index(std::string&& in_name, auto... in_ptrs) {
  auto l_table_index = type_to_table_index_[std::type_index(typeid(T))];
  auto& l_table      = static_cast<table_info<T>&>(*tables_[l_table_index]);
  unique_index_info l_unique_index{};
  l_unique_index.name_ = std::move(in_name);
  ((l_unique_index.ptrs_.push_back(
       unique_index_info::table_and_column{l_table.name_, l_table.find_column_info(in_ptrs).ptr_.name_}
   )),
   ...);
  unique_indexes_.push_back(std::move(l_unique_index));
}
}  // namespace doodle::orm