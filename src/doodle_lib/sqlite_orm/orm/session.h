#pragma once

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <boost/core/noncopyable.hpp>

#include <set>
#include <string>
#include <vector>

namespace doodle::orm {
class DOODLELIB_API session {
  struct session_data {
    sqlite_connection_ptr connection_;
    storage* s_{nullptr};
    // 嵌套事务的层数
    std::uint32_t is_transaction_{0};
    session_data() = default;
    ~session_data();
  };
  std::shared_ptr<session_data> data_;

  struct pragma_t {
    void synchronous(std::int32_t in_sync);
    void journal_mode(journal_mode_t in_mode);
    void recursive_triggers(bool in_recursive);
    void foreign_keys(bool in_foreign_keys);
    void locking_mode(bool in_exclusive);
    std::int32_t user_version();
    void user_version(std::int32_t version);

   private:
    session& s_;
    explicit pragma_t(session& s) : s_(s) {};
    friend class session;

    void run(std::string_view in_pragma_sql, bool in_value);
    void run(std::string_view in_pragma_sql, std::string_view in_value);
    void run(std::string_view in_pragma_sql, std::int32_t in_value);
  };

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

  struct transaction_guard : public boost::noncopyable {
   private:
    void begin();
    sqlite_connection_ptr connection_;
    session* s_{nullptr};
    std::int32_t transaction_size_{};

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
  // 同步schema
  void sync_schema();
  // 重建表
  // @param table_name 表名
  // @param in_columns 由于有时有新增的列, 所以需要传入新增的列名, 以便同步数据
  //
  void rebuild_table(const std::type_index& table_name, const std::vector<std::string>& in_new_columns = {});
  template <typename T>
  void rebuild_table(const std::vector<std::string>& in_new_columns = {}) {
    rebuild_table(std::type_index(typeid(T)), in_new_columns);
  }
  // 重命名表
  void rename_table(const std::string& old_name, const std::string& new_name);

  // 获取所有的表名
  std::set<std::string> get_all_table_names();
  // 获取所有的索引名
  std::set<std::string> get_all_index_names();
  // 获取所有的触发器名
  std::set<std::string> get_all_trigger_names();

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

  pragma_t pragma() { return pragma_t{*this}; }
};
}  // namespace doodle::orm