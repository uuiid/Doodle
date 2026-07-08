#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/session.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {

struct delete_t : public statement_info_base_t {
 private:
  struct delete_state_t {
    std::string from_table_name_;
    std::shared_ptr<column_operations_base_t> wheres_;
    session s_{};
    std::shared_ptr<sqlite_stmt> stmt_;
    bind_value_collector_t bind_variants_{};
  };

  friend delete_t delete_from(const session& s);

  std::shared_ptr<delete_state_t> state_;

 public:
  delete_t() : state_(std::make_shared<delete_state_t>()) {}

  template <typename T>
  delete_t where(T&& condition_fun) {
    auto l_condition_fun_ptr = std::make_shared<T>(std::forward<T>(condition_fun));
    state_->wheres_          = l_condition_fun_ptr;
    return *this;
  }
  template <typename FromTable>
  delete_t from() {
    state_->from_table_name_ = state_->s_.get_table_name<FromTable>();
    return *this;
  }

  delete_t operator()();

  std::string to_sql(const session& s, const to_sql_ctx& ctx) const override;
  void prepare(session& s, const to_sql_ctx& ctx) override;
  void collect_bind_variants(bind_value_collector_t& bind_variants) const override;
};
inline delete_t delete_from(const session& s) {
  delete_t l_delete{};
  l_delete.state_->s_ = s;
  return l_delete;
}
}  // namespace doodle::orm