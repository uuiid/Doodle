#pragma once

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <boost/core/noncopyable.hpp>

#include <optional>
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

  struct DOODLELIB_API pragma_t {
    void synchronous(std::int32_t in_sync);
    void journal_mode(journal_mode_t in_mode);
    void recursive_triggers(bool in_recursive);
    // foreign_keys 是**连接级**设置, 且 SQLite 默认为 OFF.
    // 本项目的连接级默认值由 storage::register_custom_extension 在每条连接创建时统一设置.
    void foreign_keys(bool in_foreign_keys);
    // 查询当前连接的外键开关状态.
    bool foreign_keys();
    // SQLite 3.25+ 的 ALTER TABLE RENAME 会重新解析库中所有触发器和视图.
    // 重建表时旧表刚被 DROP, 引用了它的触发器会解析失败
    // (error in trigger xxx: no such table: yyy), 需要临时开启 legacy 行为关闭该重解析.
    void legacy_alter_table(bool in_legacy);
    void locking_mode(bool in_exclusive);
    std::int32_t user_version();
    void user_version(std::int32_t version);
    // 查询外键约束违规行 (等价于 PRAGMA foreign_key_check;)
    // 必须走 PRAGMA 语句形式: 库中若存在同名真实表会遮蔽这个 eponymous 虚拟表,
    // 使 `SELECT ... FROM pragma_foreign_key_check` 静默返回 0 行.
    std::vector<detail::pragma_foreign_key_check_entry> foreign_key_check();
    // VACUUM INTO 'path': 把数据库压紧后导出到新文件 (SQLite >= 3.27).
    // 目标文件必须不存在; 不能在事务中执行.
    void vacuum_into(const FSys::path& in_path);
    // PRAGMA auto_vacuum 的查询与设置.
    // 对非空库, 设置后需要再执行一次 VACUUM (或 vacuum_into) 才真正生效;
    // 从 FULL/INCREMENTAL 降回 NONE 同样必须先 VACUUM, 否则设置不会生效.
    auto_vacuum_t auto_vacuum();
    void auto_vacuum(auto_vacuum_t in_mode);
    // PRAGMA incremental_vacuum: 归还空闲页给文件系统 (需要 auto_vacuum = incremental).
    // in_pages < 0 表示归还全部空闲页.
    // 注意: 这条语句每归还一页返回一行 (结果列数为 0), 内部会一直步进到 SQLITE_DONE.
    void incremental_vacuum(std::int32_t in_pages = -1);

   private:
    session& s_;
    explicit pragma_t(session& s) : s_(s) {};
    friend class session;

    void run(std::string_view in_pragma_sql, bool in_value);
    void run(std::string_view in_pragma_sql, std::string_view in_value);
    void run(std::string_view in_pragma_sql, std::int32_t in_value);
  };

  struct backup_t {
   private:
    sqlite3_backup* backup_{nullptr};
    sqlite_connection_ptr dest_db_{nullptr};
    sqlite_connection_ptr src_db_{};

   public:
    explicit backup_t(sqlite_connection_ptr dest_db, sqlite_connection_ptr src_db);
    std::int32_t step(int pages = -1);
    ~backup_t();

    // dis copy
    backup_t(const backup_t&)            = delete;
    backup_t& operator=(const backup_t&) = delete;

    backup_t(backup_t&&)                 = default;
    backup_t& operator=(backup_t&&)      = default;
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

  struct DOODLELIB_API transaction_guard : public boost::noncopyable {
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

  backup_t backup(const FSys::path& dest_path);
  void backup_to(const FSys::path& dest_path) {
    auto l_backup = this->backup(dest_path);
    l_backup.step(-1);
  }

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
  // WAL 检查点 (sqlite3_wal_checkpoint_v2): 把 WAL 里已提交的内容写回主库文件.
  // 库不在 WAL 模式时是空操作, 返回 {-1, -1}.
  // 注意 truncate 模式**成功时两个计数恒为 0** (SQLite 的约定: 日志已被截断成 0 字节),
  // 不代表没干活 —— 想看 WAL 里积了多少帧要用 passive.
  // 有别的连接正读着 WAL 时返回的已写回帧数会小于总帧数, 这不算失败 —— 要"全部写回"就得用
  // full / restart / truncate, 它们会等读者结束 (受 busy_timeout 限制, 未设置时不等待).
  wal_checkpoint_result_t wal_checkpoint(wal_checkpoint_mode_t in_mode = wal_checkpoint_mode_t::truncate);
  // 运行任意SQL
  void exec(std::string_view sql);
  // 同步schema
  void sync_schema();
  // 重建表
  // @param table_name 表名
  void rebuild_table(const std::type_index& table_name);
  template <typename T>
  void rebuild_table() {
    rebuild_table(std::type_index(typeid(T)));
  }
  // 重命名表
  void rename_table(const std::string& old_name, const std::string& new_name);
  // 重命名列
  void rename_column(
      const std::string& table_name, const std::string& old_column_name, const std::string& new_column_name
  );
  // 添加列, 默认值可选
  void add_column(
      const std::string& table_name, const std::string& column_name, const std::string& column_type,
      std::optional<std::string> in_default_value = std::nullopt
  );
  // 单独创建某张表
  void create_table(const std::type_index& table_name);
  template <typename T>
  void create_table() {
    create_table(std::type_index(typeid(T)));
  }

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