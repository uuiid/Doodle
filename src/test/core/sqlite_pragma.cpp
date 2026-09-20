//
// session / pragma_t 的 wal_checkpoint / vacuum_into / auto_vacuum / incremental_vacuum 测试
//

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

BOOST_AUTO_TEST_SUITE(sqlite_pragma)

namespace {

using namespace doodle;
using namespace doodle::orm;

struct pv_row {
  std::int64_t id_{};
  std::string payload_;
};

// 临时库路径 (固定名字, 每个用例开头先删除)
FSys::path temp_db(const std::string& in_tag) {
  return FSys::temp_directory_path() / fmt::format("doodle_test_pragma_{}.db", in_tag);
}

// 连同 -wal/-shm 一起删除; 用不抛异常的版本, 文件仍被连接占用时也不影响用例
void remove_db(const FSys::path& in_path) {
  std::error_code l_ec{};
  for (const auto* l_suffix : {"", "-wal", "-shm"}) FSys::remove(FSys::path{in_path.generic_string() + l_suffix}, l_ec);
}

// 用一个全新的连接读取库状态, 判断设置是否真的落到了库文件上
std::int32_t auto_vacuum_via_new_connection(const FSys::path& in_db) {
  sqlite_storage l_probe{};
  l_probe.open(in_db);
  auto l_session = l_probe.create_session();
  return static_cast<std::int32_t>(l_session.pragma().auto_vacuum());
}

std::int64_t row_count_via_new_connection(const FSys::path& in_db) {
  sqlite_storage l_probe{};
  l_probe.open(in_db);
  auto l_session = l_probe.create_session();
  sqlite_stmt l_stmt{l_session, "SELECT count(*) FROM pv_row"};
  l_stmt.step();
  return l_stmt.get_column_value<std::int64_t>(0);
}

void reg_pv_table(sqlite_storage& in_storage) {
  in_storage.reg_table<pv_row>("pv_row")
      .add_column("id", &pv_row::id_, primary_key())
      .add_column("payload", &pv_row::payload_);
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

}  // namespace

// vacuum_into 导出的副本内容一致且结构完好, 源库不受影响
BOOST_AUTO_TEST_CASE(vacuum_into_produces_usable_copy) {
  app_base l_app{};
  auto l_src = temp_db("vacuum_src");
  auto l_out = temp_db("vacuum_out");
  remove_db(l_src);
  remove_db(l_out);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_src);
    reg_pv_table(l_storage);
    auto l_session = l_storage.create_session();
    l_session.create_table<pv_row>();
    {
      auto l_guard = l_session.transaction();
      for (std::int64_t l_i = 1; l_i <= 200; ++l_i)
        insert(l_session).into<pv_row>().set(c(&pv_row::id_) = l_i, c(&pv_row::payload_) = "payload")();
      l_guard.commit();
    }
    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM pv_row") == 200);

    l_session.pragma().vacuum_into(l_out);
    BOOST_REQUIRE(FSys::exists(l_out));

    // 导出之后源库仍然可用
    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM pv_row") == 200);
  }

  {
    sqlite_storage l_out_storage{};
    l_out_storage.open(l_out);
    auto l_session = l_out_storage.create_session();
    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM pv_row") == 200);
    BOOST_TEST(scalar_text(l_session, "SELECT payload FROM pv_row WHERE id = 7") == "payload");
    BOOST_TEST(scalar_text(l_session, "PRAGMA integrity_check;") == "ok");
  }

  remove_db(l_src);
  remove_db(l_out);
}

// auto_vacuum 的查询与设置: 空库上设置立即生效
BOOST_AUTO_TEST_CASE(auto_vacuum_get_set) {
  app_base l_app{};
  auto l_db = temp_db("autovacuum");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    auto l_session = l_storage.create_session();

    // 新库默认 NONE
    BOOST_TEST(static_cast<std::int32_t>(l_session.pragma().auto_vacuum()) == 0);

    l_session.pragma().auto_vacuum(auto_vacuum_t::incremental);
    BOOST_TEST(static_cast<std::int32_t>(l_session.pragma().auto_vacuum()) == 2);

    l_session.pragma().auto_vacuum(auto_vacuum_t::full);
    BOOST_TEST(static_cast<std::int32_t>(l_session.pragma().auto_vacuum()) == 1);

    // SQLite 不允许从 FULL/INCREMENTAL 直接降回 NONE, 必须 VACUUM 之后才生效
    l_session.pragma().auto_vacuum(auto_vacuum_t::none);
    BOOST_TEST(static_cast<std::int32_t>(l_session.pragma().auto_vacuum()) == 1);
    l_session.vacuum();
    BOOST_TEST(static_cast<std::int32_t>(l_session.pragma().auto_vacuum()) == 0);
  }

  remove_db(l_db);
}

// incremental_vacuum 把空闲页归还给文件系统
BOOST_AUTO_TEST_CASE(incremental_vacuum_frees_pages) {
  app_base l_app{};
  auto l_db = temp_db("incvacuum");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    reg_pv_table(l_storage);
    auto l_session = l_storage.create_session();

    // 必须在建表之前设为 incremental (否则要 VACUUM 一次才生效), 这里再 VACUUM 保险
    l_session.pragma().auto_vacuum(auto_vacuum_t::incremental);
    l_session.vacuum();
    l_session.create_table<pv_row>();

    // 写入较多数据再删掉大部分, 制造空闲页
    {
      auto l_guard = l_session.transaction();
      for (std::int64_t l_i = 1; l_i <= 3000; ++l_i)
        insert(l_session).into<pv_row>().set(c(&pv_row::id_) = l_i, c(&pv_row::payload_) = std::string(200, 'x'))();
      l_session.exec("DELETE FROM pv_row WHERE id > 100;");
      l_guard.commit();
    }

    // 模式确实写进了库文件头, 且数据确实已提交 (另一个连接可见)
    BOOST_TEST(auto_vacuum_via_new_connection(l_db) == 2);
    BOOST_TEST(row_count_via_new_connection(l_db) == 100);

    auto l_free_before = scalar_int(l_session, "PRAGMA freelist_count;");
    BOOST_TEST_MESSAGE(fmt::format("incremental_vacuum 前空闲页 = {}", l_free_before));
    BOOST_TEST(l_free_before > 0);

    // 不带参数: 归还全部空闲页
    l_session.pragma().incremental_vacuum();
    auto l_free_after = scalar_int(l_session, "PRAGMA freelist_count;");
    BOOST_TEST_MESSAGE(fmt::format("incremental_vacuum 后空闲页 = {}", l_free_after));
    BOOST_TEST(l_free_after == 0);

    // 带页数参数: 重新制造空闲页, 验证只归还请求的页数
    {
      auto l_guard = l_session.transaction();
      for (std::int64_t l_i = 1000; l_i <= 4000; ++l_i)
        insert(l_session).into<pv_row>().set(c(&pv_row::id_) = l_i, c(&pv_row::payload_) = std::string(200, 'x'))();
      l_session.exec("DELETE FROM pv_row WHERE id >= 1000;");
      l_guard.commit();
    }
    auto l_free_more = scalar_int(l_session, "PRAGMA freelist_count;");
    BOOST_TEST_MESSAGE(fmt::format("重新制造的空闲页 = {}", l_free_more));
    BOOST_TEST(l_free_more > 10);

    l_session.pragma().incremental_vacuum(10);
    auto l_free_limited = scalar_int(l_session, "PRAGMA freelist_count;");
    BOOST_TEST_MESSAGE(fmt::format("incremental_vacuum(10) 后空闲页 = {}", l_free_limited));
    BOOST_TEST(l_free_limited < l_free_more);
    BOOST_TEST(l_free_more - l_free_limited <= 10);

    // 再次不带参数, 清空剩余空闲页
    l_session.pragma().incremental_vacuum();
    BOOST_TEST(scalar_int(l_session, "PRAGMA freelist_count;") == 0);

    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM pv_row") == 100);
    BOOST_TEST(scalar_text(l_session, "PRAGMA integrity_check;") == "ok");
  }

  remove_db(l_db);
}

// wal_checkpoint: WAL 模式下把 WAL 写回主库并截断; 非 WAL 模式下是空操作
BOOST_AUTO_TEST_CASE(wal_checkpoint_flushes_and_truncates) {
  app_base l_app{};
  auto l_db = temp_db("walckpt");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    reg_pv_table(l_storage);
    auto l_session = l_storage.create_session();
    l_session.create_table<pv_row>();

    // 还没切到 WAL: 检查点什么都不做, 两个计数都保持 -1 —— 这是 SQLite 在没开 WAL 时的约定,
    // 也正是"升级最后那次检查点必须放在 journal_mode(WAL) 之后"的原因
    auto l_none = l_session.wal_checkpoint(wal_checkpoint_mode_t::truncate);
    BOOST_TEST(l_none.log_frames_ == -1);
    BOOST_TEST(l_none.checkpointed_frames_ == -1);

    l_session.pragma().journal_mode(journal_mode_t::wal);
    BOOST_TEST(scalar_text(l_session, "PRAGMA journal_mode;") == "wal");

    // 写一批数据, 让 WAL 里确实有内容
    {
      auto l_guard = l_session.transaction();
      for (std::int64_t l_i = 1; l_i <= 2000; ++l_i)
        insert(l_session).into<pv_row>().set(c(&pv_row::id_) = l_i, c(&pv_row::payload_) = std::string(200, 'y'))();
      l_guard.commit();
    }

    // 先 passive 看一眼 WAL 里到底积了多少帧 —— truncate 成功后按 SQLite 的约定返回 0/0,
    // 光看它看不出检查点到底干了多少活
    auto l_passive = l_session.wal_checkpoint(wal_checkpoint_mode_t::passive);
    BOOST_TEST_MESSAGE(
        fmt::format("passive: 总帧 {} / 已写回 {}", l_passive.log_frames_, l_passive.checkpointed_frames_)
    );
    BOOST_TEST(l_passive.log_frames_ >= 0);
    BOOST_TEST(l_passive.checkpointed_frames_ == l_passive.log_frames_);

    // truncate: 成功后 WAL 被截断成 0 字节, 两个计数按约定都是 0
    auto l_ckpt = l_session.wal_checkpoint(wal_checkpoint_mode_t::truncate);
    BOOST_TEST_MESSAGE(fmt::format("truncate: 总帧 {} / 已写回 {}", l_ckpt.log_frames_, l_ckpt.checkpointed_frames_));
    BOOST_TEST(l_ckpt.log_frames_ == 0);
    BOOST_TEST(l_ckpt.checkpointed_frames_ == 0);

    // truncate 之后 WAL 必须是空的 (文件被整个删掉也算空)
    auto l_wal = FSys::path{l_db.generic_string() + "-wal"};
    std::error_code l_ec{};
    auto l_wal_size = FSys::exists(l_wal, l_ec) ? FSys::file_size(l_wal, l_ec) : std::uintmax_t{0};
    BOOST_TEST_MESSAGE(fmt::format("检查点后 WAL 大小 = {}", l_wal_size));
    BOOST_TEST(l_wal_size == 0);

    // 数据没丢, 模式也没被改掉
    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM pv_row") == 2000);
    BOOST_TEST(scalar_text(l_session, "PRAGMA journal_mode;") == "wal");
    BOOST_TEST(scalar_text(l_session, "PRAGMA integrity_check;") == "ok");
  }

  remove_db(l_db);
}

BOOST_AUTO_TEST_SUITE_END()
