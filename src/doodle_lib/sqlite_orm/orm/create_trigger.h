#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <memory>
#include <string>
#include <vector>

namespace doodle::orm {

struct create_trigger_t {
  std::shared_ptr<trigger_info> info_;
  storage* s_{nullptr};

 public:
  explicit create_trigger_t(std::string in_name, std::shared_ptr<trigger_info> info, storage* s)
      : info_(std::move(info)), s_(s) {
    info_->name_ = std::move(in_name);
  }
  create_trigger_t& before() {
    info_->timing_ = trigger_timing::before;
    return *this;
  }
  create_trigger_t& after() {
    info_->timing_ = trigger_timing::after;
    return *this;
  }
  create_trigger_t& instead_of() {
    info_->timing_ = trigger_timing::instead_of;
    return *this;
  }
  create_trigger_t& on(std::string in_table_name) {
    info_->table_name_ = std::move(in_table_name);
    return *this;
  }
  create_trigger_t& delete_() {
    info_->event_ = trigger_event::delete_;
    return *this;
  }
  create_trigger_t& insert() {
    info_->event_ = trigger_event::insert;
    return *this;
  }
  template <typename... Columns>
  create_trigger_t& update_of(Columns&&... in_columns) {
    info_->event_   = trigger_event::update;
    auto l_iter_fun = [this](auto&& in_column) {
      using column_or_struct_type = std::decay_t<decltype(in_column)>;
      if constexpr (std::is_member_pointer_v<std::decay_t<decltype(in_column)>>) {
        info_->columns_.push_back(s_->get_column_name(in_column));
      } else {
        static_assert(always_false<column_or_struct_type>, "不支持的参数类型");
      }
    };
    (l_iter_fun(in_columns), ...);
    return *this;
  }
  create_trigger_t& begin() { return *this; }
  create_trigger_t& end() { return *this; }

  create_trigger_t& statement(const update_t& in_statement);
  create_trigger_t& statement(const delete_t& in_statement);
  create_trigger_t& statement(const insert_t& in_statement);

  };

}  // namespace doodle::orm