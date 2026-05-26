#pragma once
#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/table_info.h>

#include <memory>
#include <string>

namespace doodle::orm {
struct create_index_base_t {
  struct impl {
    std::string name_;
    table_info_base_ptr table_name_;
    std::vector<column_info_ptr> column_names_;
    bool unique_{false};
    std::shared_ptr<column_operations_base_t> on_condition_;
  };
  std::shared_ptr<impl> info_;

 public:
  struct index_info {
    std::string name_;
    std::string table_name_;
    std::vector<std::string> column_names_;
    bool unique_{false};
    bool has_condition_{false};
    bool operator==(const index_info& other) const {
      if (table_name_ != other.table_name_) return false;
      if (column_names_.size() != other.column_names_.size()) return false;
      for (size_t i = 0; i < column_names_.size(); ++i) {
        if (column_names_[i] != other.column_names_[i]) return false;
      }
      if (unique_ != other.unique_) return false;
      if (has_condition_ != other.has_condition_) return false;
      return true;
    }
    bool operator!=(const index_info& other) const { return !(*this == other); }

    bool operator<(const index_info& other) const {
      if (table_name_ != other.table_name_) return table_name_ < other.table_name_;
      if (column_names_.size() != other.column_names_.size()) return column_names_.size() < other.column_names_.size();
      for (size_t i = 0; i < column_names_.size(); ++i) {
        if (column_names_[i] != other.column_names_[i]) return column_names_[i] < other.column_names_[i];
      }
      if (unique_ != other.unique_) return unique_ < other.unique_;
      if (has_condition_ != other.has_condition_) return has_condition_ < other.has_condition_;
      return false;
    }
    bool operator>(const index_info& other) const { return other < *this; }
    bool operator<=(const index_info& other) const { return !(other < *this); }
    bool operator>=(const index_info& other) const { return !(*this < other); }
  };
  create_index_base_t() : info_(std::make_shared<impl>()) {}

  ~create_index_base_t() = default;
  std::string to_sql(storage& s, to_sql_ctx ctx) const;

  template <typename Table>
  create_index_base_t table() {
    info_->table_name_ = std::make_shared<table_info_t>(typeid(Table));
    return *this;
  }

  template <typename Table>
  create_index_base_t on(auto Table::*... in_ptr) {
    auto l_iter_fun = [this](auto&& in_column) {
      using column_or_struct_type = std::decay_t<decltype(in_column)>;
      if constexpr (std::is_member_pointer_v<std::decay_t<decltype(in_column)>>) {
        info_->column_names_.push_back(std::make_shared<column_info_t>(in_column));
      } else {
        static_assert(always_false<column_or_struct_type>, "不支持的参数类型");
      }
    };
    (l_iter_fun(in_ptr), ...);
    return *this;
  }

  create_index_base_t& unique() {
    info_->unique_ = true;
    return *this;
  }
  template <typename T>
  create_index_base_t& where(T&& condition_fun) {
    auto l_condition_fun_ptr = std::make_shared<std::decay_t<T>>(std::forward<T>(condition_fun));
    info_->on_condition_     = l_condition_fun_ptr;
    return *this;
  }

  index_info get_index_info(storage& s, to_sql_ctx ctx) const;
};
template <typename Table>
create_index_base_t create_index() {
  create_index_base_t l_index{};
  l_index.table<Table>();
  return l_index;
}
template <typename Table>
create_index_base_t create_unique_index() {
  create_index_base_t index{};
  index.table<Table>();
  index.unique();
  return index;
}

}  // namespace doodle::orm