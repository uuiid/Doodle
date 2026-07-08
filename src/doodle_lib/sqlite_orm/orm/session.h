#pragma once

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {
class session {
  struct session_data {
    sqlite_connection_ptr connection_;
    storage* s_{nullptr};
    session_data() = default;
    ~session_data();
  };
  std::shared_ptr<session_data> data_;

 public:
  explicit session(storage& s);
  session() : data_(std::make_shared<session_data>()) {}
  ~session()                         = default;

  // dis copy
  session(const session&)            = default;
  session& operator=(const session&) = default;

  // default move
  session(session&&)                 = default;
  session& operator=(session&&)      = default;

  operator bool() const { return data_ && data_->connection_ && data_->s_; }

  struct transaction_guard {
    void begin();
    sqlite_connection_ptr connection_;

   public:
    bool committed_{false};
    explicit transaction_guard(session& s);
    void commit();
    void rollback();
    ~transaction_guard();
  };

  transaction_guard transaction();
  sqlite_connection_ptr get_connection() const;

  // 删除表
  void drop_table(const std::string& table_name);
  // 删除索引
  void drop_index(const std::string& index_name);
  // 删除触发器
  void drop_trigger(const std::string& trigger_name);
  // 删除view
  void drop_view(const std::string& view_name);
  // 检查表是否存在
  bool table_exists(const std::string& table_name);
  // 检查索引是否存在
  bool index_exists(const std::string& index_name);
  // 检查触发器是否存在
  bool trigger_exists(const std::string& trigger_name);
  // vacuum数据库
  void vacuum();
  // 运行任意SQL
  void exec(std::string_view sql);

  // 储存接口
  template <typename T>
  bool has_reg_table() {
    return data_->s_->has_reg_table<T>();
  }

  template <typename T>
  std::string get_column_name(auto T::* in_ptr, const to_sql_ctx& ctx) const {
    return data_->s_->get_column_name(in_ptr, ctx);
  }
  std::string get_column_name(const table_columns_t& in_column, const to_sql_ctx& ctx) const {
    return data_->s_->get_column_name(in_column, ctx);
  }

  template <typename T>
  const std::vector<column_info>& get_table_columns() const {
    return data_->s_->get_table_columns<T>();
  }
  template <typename T>
  std::string get_table_name() const {
    return data_->s_->get_table_name<T>();
  }
  std::string get_table_name(std::type_index in_type_index) const { return data_->s_->get_table_name(in_type_index); }
};
}  // namespace doodle::orm