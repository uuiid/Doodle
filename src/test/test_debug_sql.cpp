#include "test_debug_sql.h"

#include <doodle_core/lib_warp/json_warp.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/http/http_body.h>
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <sqlite3.h>

namespace doodle::http {

namespace {

struct exec_result_t {
  std::int32_t code_{SQLITE_OK};
  std::string message_{};
};

int collect_row(void* in_ctx, int in_argc, char** in_argv, char** in_col_names) {
  auto& l_rows = *static_cast<nlohmann::json::array_t*>(in_ctx);
  auto l_row   = nlohmann::json::object();
  for (int i = 0; i < in_argc; ++i) {
    const auto l_name = std::string{in_col_names[i] == nullptr ? "column" : in_col_names[i]};
    if (in_argv[i] == nullptr) {
      l_row[l_name] = nullptr;
    } else {
      l_row[l_name] = std::string{in_argv[i]};
    }
  }
  l_rows.emplace_back(std::move(l_row));
  return SQLITE_OK;
}

exec_result_t run_sql(sqlite3* in_db, const std::string& in_sql, nlohmann::json::array_t& io_rows) {
  exec_result_t l_result{};
  char* l_err_msg = nullptr;
  l_result.code_  = sqlite3_exec(in_db, in_sql.c_str(), collect_row, &io_rows, &l_err_msg);
  if (l_result.code_ != SQLITE_OK) {
    l_result.message_ = l_err_msg == nullptr ? sqlite3_errmsg(in_db) : l_err_msg;
    sqlite3_free(l_err_msg);
  }
  return l_result;
}

nlohmann::json make_error_json(const exec_result_t& in_result) {
  return nlohmann::json{
      {"code",    in_result.code_                          },
      {"name",    std::string{sqlite3_errstr(in_result.code_)}},
      {"message", in_result.message_                       },
  };
}

}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(debug_sql, post) {
  auto l_json = in_handle->get_json();
  DOODLE_CHICK_HTTP(
      l_json.contains("sql") && l_json["sql"].is_string(), bad_request,
      "缺少 sql 字段, 请求体需要 JSON 对象 {{\"sql\": \"...\"}}"
  );
  const auto l_sql         = l_json["sql"].get<std::string>();
  const auto l_use_tx      = l_json.value("transaction", true);
  const auto l_stop_on_err = l_json.value("stop_on_error", true);

  auto l_this_executor = co_await boost::asio::this_coro::executor;
  auto& l_storage      = *core_set::get_set().database_;
  co_await boost::asio::dispatch(boost::asio::bind_executor(l_storage.get_strand(), boost::asio::use_awaitable));

  auto l_session = l_storage.create_session();
  auto l_conn    = l_session.get_connection();
  sqlite3* l_db  = *l_conn;

  const auto l_changes_before = sqlite3_total_changes(l_db);
  const auto l_start          = std::chrono::steady_clock::now();
  nlohmann::json::array_t l_rows{};

  exec_result_t l_begin{};
  if (l_use_tx) {
    nlohmann::json::array_t l_ignored{};
    l_begin = run_sql(l_db, "BEGIN IMMEDIATE;", l_ignored);
    DOODLE_CHICK_HTTP(
        l_begin.code_ == SQLITE_OK, internal_server_error, "开启事务失败: {}", make_error_json(l_begin).dump()
    );
  }

  auto l_exec = run_sql(l_db, l_sql, l_rows);
  if (l_exec.code_ != SQLITE_OK && !l_stop_on_err) {
    default_logger_raw()->warn("debug_sql 忽略错误继续执行: {}", make_error_json(l_exec).dump());
  }

  exec_result_t l_end{};
  if (l_use_tx) {
    nlohmann::json::array_t l_ignored{};
    const auto l_end_sql = l_exec.code_ == SQLITE_OK ? "COMMIT;" : "ROLLBACK;";
    l_end                = run_sql(l_db, l_end_sql, l_ignored);
    DOODLE_CHICK_HTTP(
        l_end.code_ == SQLITE_OK, internal_server_error, "{}失败: {}",
        l_exec.code_ == SQLITE_OK ? "提交" : "回滚", make_error_json(l_end).dump()
    );
  }

  const auto l_elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - l_start);

  nlohmann::json l_result       = nlohmann::json::object();
  l_result["ok"]                = l_exec.code_ == SQLITE_OK;
  l_result["rolled_back"]       = l_use_tx && l_exec.code_ != SQLITE_OK;
  l_result["error"]             = make_error_json(l_exec);
  l_result["changes"]           = sqlite3_total_changes(l_db) - l_changes_before;
  l_result["total_changes"]     = sqlite3_total_changes(l_db);
  l_result["last_insert_rowid"] = sqlite3_last_insert_rowid(l_db);
  l_result["elapsed_ms"]        = l_elapsed.count();
  l_result["row_count"]         = l_rows.size();
  l_result["rows"]              = std::move(l_rows);

  co_await boost::asio::dispatch(boost::asio::bind_executor(l_this_executor, boost::asio::use_awaitable));

  auto l_body = std::make_unique<http_body_json_t>(
      std::move(l_result),
      l_exec.code_ == SQLITE_OK ? boost::beast::http::status::ok : boost::beast::http::status::bad_request
  );
  co_return l_body;
}

}  // namespace doodle::http
