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
  explicit create_index_base_t(std::string in_name) : info_(std::make_shared<impl>()) {
    info_->name_ = std::move(in_name);
  }

  ~create_index_base_t() = default;
  std::string to_sql(storage& s, to_sql_ctx ctx) const;

  template <typename Table>
  create_index_base_t on(auto Table::*... in_ptr) {
    info_->table_name_ = std::make_shared<table_info_t>(typeid(Table));
    auto l_iter_fun    = [this](auto&& in_column) {
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
};

}  // namespace doodle::orm