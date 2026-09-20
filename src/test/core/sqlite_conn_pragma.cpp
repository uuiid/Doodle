//
// 连接级 PRAGMA 与外键声明不变量的断言.
//
// 背景: foreign_keys / synchronous / recursive_triggers 都是**连接级**设置, 而 SQLite 的
// foreign_keys 默认为 OFF. 连接由连接池复用, 所以这类设置必须在每条连接创建时设置
// (storage::register_custom_extension 由 only_open_db 对每条连接调用).
// 曾经这些设置是用一个临时 session 去设的, 只能落到连接池中恰好被借到的那一条连接上,
// 于是外键约束在大部分连接上静默失效, 违规数据长期累积而从不报错.
//

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <system_error>
#include <vector>

BOOST_AUTO_TEST_SUITE(sqlite_conn_pragma)

namespace {

using namespace doodle;
using namespace doodle::orm;

FSys::path temp_db(const std::string& in_tag) {
  return FSys::temp_directory_path() / fmt::format("doodle_test_conn_{}.db", in_tag);
}

void remove_db(const FSys::path& in_path) {
  std::error_code l_ec{};
  for (const auto* l_suffix : {"", "-wal", "-shm"}) FSys::remove(FSys::path{in_path.generic_string() + l_suffix}, l_ec);
}

// 执行一条不返回结果的语句 (建表/插入等)
void exec_sql(orm::session& in_session, const std::string& in_sql) {
  sqlite_stmt l_stmt{in_session, in_sql};
  l_stmt.step();
}

std::vector<std::vector<std::string>> query_rows(
    orm::session& in_session, const std::string& in_sql, std::size_t in_cols
) {
  sqlite_stmt l_stmt{in_session, in_sql};
  std::vector<std::vector<std::string>> l_result{};
  while (l_stmt.step_not_throw() == SQLITE_ROW) {
    std::vector<std::string> l_row{};
    l_row.reserve(in_cols);
    for (std::size_t l_i = 0; l_i < in_cols; ++l_i) {
      l_row.push_back(l_stmt.column_is_null(static_cast<int>(l_i)) ? std::string{}
                                                                 : l_stmt.get_column_value<std::string>(l_i));
    }
    l_result.push_back(std::move(l_row));
  }
  return l_result;
}

std::int64_t scalar_int(orm::session& in_session, const std::string& in_sql) {
  sqlite_stmt l_stmt{in_session, in_sql};
  l_stmt.step();
  return l_stmt.get_column_value<std::int64_t>(0);
}

std::string scalar_text(orm::session& in_session, const std::string& in_sql) {
  sqlite_stmt l_stmt{in_session, in_sql};
  l_stmt.step();
  return l_stmt.get_column_value<std::string>(0);
}

struct column_meta {
  bool not_null_{false};
  bool primary_key_{false};
};

struct table_meta {
  std::map<std::string, column_meta> columns_{};
  std::set<std::string> unique_columns_{};
};

// 读取整库的列约束与唯一索引, 供外键不变量检查使用
std::map<std::string, table_meta> read_schema_meta(orm::session& in_session) {
  std::map<std::string, table_meta> l_result{};
  auto l_tables = query_rows(
      in_session, "SELECT name FROM sqlite_master WHERE type = 'table' AND name NOT LIKE 'sqlite_%';", 1
  );
  for (const auto& l_table : l_tables) {
    const auto& l_name = l_table[0];
    table_meta l_meta{};
    for (const auto& l_r : query_rows(
             in_session, fmt::format("SELECT name, \"notnull\", pk FROM pragma_table_info('{}');", l_name), 3
         )) {
      l_meta.columns_[l_r[0]] = column_meta{l_r[1] == "1", l_r[2] == "1"};
    }
    for (const auto& l_r : query_rows(
             in_session,
             fmt::format(
                 "SELECT il.\"unique\", ii.name FROM pragma_index_list('{}') il "
                 "JOIN pragma_index_info(il.name) ii;",
                 l_name
             ),
             2
         )) {
      if (l_r[0] == "1") l_meta.unique_columns_.insert(l_r[1]);
    }
    l_result[l_name] = std::move(l_meta);
  }
  return l_result;
}

}  // namespace

// 每条连接都必须开启外键约束.
// 同时持有多个 session 时, get_thread_db() 在池空的情况下会新建连接,
// 因此这些 session 必然落在**不同**的连接上 —— 正好覆盖"设置只落到一条连接"这个退化情况.
BOOST_AUTO_TEST_CASE(every_connection_has_foreign_keys_on) {
  app_base l_app{};
  auto l_db = temp_db("fk_on");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);

    auto l_s1 = l_storage.create_session();
    auto l_s2 = l_storage.create_session();
    auto l_s3 = l_storage.create_session();
    auto l_s4 = l_storage.create_session();

    for (auto* l_p : {&l_s1, &l_s2, &l_s3, &l_s4}) {
      BOOST_TEST(l_p->pragma().foreign_keys());
    }

    // 只看 PRAGMA 的返回值还不够, 这里再验证约束真的在拦写入
    exec_sql(l_s1, "CREATE TABLE parent(id INTEGER PRIMARY KEY, u TEXT UNIQUE);");
    exec_sql(l_s1, "CREATE TABLE child(id INTEGER PRIMARY KEY, p TEXT REFERENCES parent(u));");
    exec_sql(l_s1, "INSERT INTO parent(id, u) VALUES (1, 'a');");
    exec_sql(l_s1, "INSERT INTO child(id, p) VALUES (1, 'a');");

    bool l_rejected{false};
    try {
      exec_sql(l_s1, "INSERT INTO child(id, p) VALUES (2, 'no_such_parent');");
    } catch (const std::exception&) {
      l_rejected = true;
    }
    BOOST_TEST_MESSAGE("违反外键的写入被拒绝: " + std::string{l_rejected ? "是" : "否"});
    BOOST_TEST(l_rejected);
  }

  remove_db(l_db);
}

// 外键声明的两条不变量, 在 regs_all() 生成的整套 schema 上全局成立:
//   1. ON DELETE SET NULL 不能配在 NOT NULL 列上 —— 删父行时置 NULL 会撞 NOT NULL 约束而失败;
//   2. 外键必须指向父表的主键或唯一列 —— 否则 DML 时报 "foreign key mismatch".
// 这两条曾各自出过问题 (comment.person_id / seedance2_subproject.created_user_id), 见 regs_all().
BOOST_AUTO_TEST_CASE(fk_declaration_invariants) {
  app_base l_app{};
  auto l_db = temp_db("invariants");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    auto l_session = l_storage.create_session();
    l_session.sync_schema();

    auto l_meta = read_schema_meta(l_session);

    std::vector<std::string> l_set_null_on_not_null{};
    std::vector<std::string> l_non_unique_parent{};
    std::size_t l_fk_total{0};

    for (const auto& [l_table, l_table_meta] : l_meta) {
      for (const auto& l_r : query_rows(
               l_session,
               fmt::format(
                   "SELECT \"from\", \"table\", \"to\", on_delete FROM pragma_foreign_key_list('{}');", l_table
               ),
               4
           )) {
        ++l_fk_total;
        const auto& l_from      = l_r[0];
        const auto& l_parent    = l_r[1];
        const auto& l_to        = l_r[2];
        const auto& l_on_delete = l_r[3];

        if (l_on_delete == "SET NULL") {
          auto l_it = l_table_meta.columns_.find(l_from);
          if (l_it != l_table_meta.columns_.end() && l_it->second.not_null_) {
            l_set_null_on_not_null.push_back(fmt::format("{}.{} -> {}", l_table, l_from, l_parent));
          }
        }

        auto l_parent_it = l_meta.find(l_parent);
        if (l_parent_it != l_meta.end()) {
          auto l_col_it = l_parent_it->second.columns_.find(l_to);
          const bool l_is_pk = l_col_it != l_parent_it->second.columns_.end() && l_col_it->second.primary_key_;
          const bool l_is_unique = l_parent_it->second.unique_columns_.contains(l_to);
          if (!l_is_pk && !l_is_unique) {
            l_non_unique_parent.push_back(
                fmt::format("{}.{} -> {}.{}", l_table, l_from, l_parent, l_to)
            );
          }
        }
      }
    }

    BOOST_TEST_MESSAGE(fmt::format("共检查 {} 个外键", l_fk_total));
    for (const auto& l_s : l_set_null_on_not_null) BOOST_TEST_MESSAGE("SET NULL 但列 NOT NULL: " + l_s);
    for (const auto& l_s : l_non_unique_parent) BOOST_TEST_MESSAGE("外键指向非唯一父列: " + l_s);

    BOOST_TEST(l_fk_total > 100);
    BOOST_TEST(l_set_null_on_not_null.empty());
    BOOST_TEST(l_non_unique_parent.empty());
  }

  remove_db(l_db);
}

// 全新库 (user_version == 0) 的初始化路径必须在外键开启的情况下走通.
// upgrade_init_t 会 sync_schema() 建全部表并插入内置常量行 (project_status / asset_type /
// task_type / assets), 这条路径此前只在外键默认关闭时跑过, 是开启外键后最需要回归的一环.
// 关键前提: 可空引用列未赋值时是 nil uuid, 由 sqlite_statement.h 绑定成 NULL, 而 NULL 永远
// 满足外键约束 —— 所以未赋值的可选引用不会误报违规.
BOOST_AUTO_TEST_CASE(fresh_db_upgrade_succeeds_with_fk_enforced) {
  app_base l_app{};
  auto l_db = temp_db("fresh_upgrade");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    l_storage.upgrade();

    auto l_session = l_storage.create_session();
    auto l_version = scalar_int(l_session, "PRAGMA user_version;");
    BOOST_TEST_MESSAGE(fmt::format("全新库 user_version = {}", l_version));
    BOOST_TEST(l_version > 0);

    auto l_task_type_count = scalar_int(l_session, "SELECT count(*) FROM task_type;");
    BOOST_TEST_MESSAGE(fmt::format("内置 task_type 常量行数 = {}", l_task_type_count));
    BOOST_TEST(l_task_type_count >= 11);

    BOOST_TEST(scalar_text(l_session, "PRAGMA integrity_check;") == "ok");
    // 全新库插入完常量后不应产生任何外键违规
    auto l_bad = l_session.pragma().foreign_key_check();
    for (const auto& l_entry : l_bad) {
      BOOST_TEST_MESSAGE(fmt::format("全新库外键违规: {} rowid={} -> {}", l_entry.table, l_entry.rowid, l_entry.parent));
    }
    BOOST_TEST(l_bad.empty());
  }

  remove_db(l_db);
}

BOOST_AUTO_TEST_SUITE_END()
