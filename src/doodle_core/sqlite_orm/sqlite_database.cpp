//
// Created by TD on 24-9-12.
//

#include "sqlite_database.h"

#include <doodle_core/core/app_base.h>

#include <sqlite_orm/sqlite_orm.h>
#include <sqlite_orm/uuid_to_blob.h>
namespace doodle {

namespace {
auto make_storage_doodle(const std::string& in_path) {
  using namespace sqlite_orm;

  return make_storage(
      in_path,  //
      make_table(
          "scan_data",  //
          make_column("id", &scan_data_t::database_t::id_, primary_key()),
          make_column("ue_uuid", &scan_data_t::database_t::ue_uuid_, unique()),
          make_column("rig_uuid", &scan_data_t::database_t::rig_uuid_, unique()),
          make_column("solve_uuid", &scan_data_t::database_t::solve_uuid_, unique()),

          make_column("ue_path", &scan_data_t::database_t::ue_path_),
          make_column("rig_path", &scan_data_t::database_t::rig_path_),
          make_column("solve_path", &scan_data_t::database_t::solve_path_),

          make_column("project", &scan_data_t::database_t::project_),
          make_column("num", &scan_data_t::database_t::num_),  //
          make_column("name", &scan_data_t::database_t::name_),
          make_column("version", &scan_data_t::database_t::version_)
      ),  //
      make_table(
          "project_tab",                                                       //
          make_column("id", &project_helper::database_t::id_, primary_key()),  //
          make_column("uuid_id", &project_helper::database_t::uuid_id_, unique()),
          make_column("name", &project_helper::database_t::name_),      //
          make_column("en_str", &project_helper::database_t::en_str_),  //
          make_column("shor_str", &project_helper::database_t::shor_str_),
          make_column("local_path", &project_helper::database_t::local_path_),
          make_column("auto_upload_path", &project_helper::database_t::auto_upload_path_)
      )

  );
}
using sqlite_orm_type = decltype(make_storage_doodle(""));
}  // namespace

void sqlite_database::set_path(const FSys::path& in_path) {
  auto l_storage = make_storage_doodle(in_path.generic_string());
  l_storage.sync_schema(true);
  storage_any_ = std::move(l_storage);
}

void sqlite_database::load(const FSys::path& in_path) {
  set_path(in_path);

#define DOODLE_CREATE_ID(clas_)                                                                           \
  if (auto l_data = l_storage.get_all<clas_::database_t>(); !l_data.empty()) {                            \
    auto l_create = clas_::load_from_sql(*g_reg(), l_data);                                               \
    std::vector<entt::id_type> l_vec =                                                                    \
        l_data | ranges::views::transform([](const auto& in_db) -> entt::id_type { return in_db.id_; }) | \
        ranges::to_vector;                                                                                \
    l_s.insert(l_create.begin(), l_create.end(), l_vec.begin());                                          \
  }

  auto l_storage = std::any_cast<sqlite_orm_type&>(storage_any_);
  auto& l_s      = g_reg()->storage<entt::id_type>(detail::sql_id);
  DOODLE_CREATE_ID(project_helper);
  DOODLE_CREATE_ID(scan_data_t);

#undef DOODLE_CREATE_ID
}

void sqlite_database::run() {
  strand_ = std::make_shared<strand_type>(boost::asio::make_strand(g_io_context()));
  timer_  = std::make_shared<timer_type>(*strand_);
  boost::asio::co_spawn(
      *strand_, run_impl(), boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached)
  );
}
boost::asio::awaitable<void> sqlite_database::run_impl() {
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    save();
    timer_->expires_after(1s);
    auto [l_ec] = co_await timer_->async_wait();
    if (l_ec) {
      break;
    }
  }
  co_return;
}

void sqlite_database::save() {
  auto& l_sqlite  = std::any_cast<sqlite_orm_type&>(storage_any_);
  auto& l_storage = g_reg()->storage<entt::id_type>(detail::sql_id);

#define DOODLE_SAVE_ID(clas_, method_)                                                        \
  {                                                                                           \
    clas_::database_t l_out{};                                                                \
    while ((method_).pop(l_out)) {                                                            \
      entt::entity l_entt{boost::numeric_cast<entt::id_type>(l_out.id_)};                     \
      if (l_storage.get(l_entt) == 0) {                                                       \
        l_storage.patch(l_entt) = boost::numeric_cast<entt::id_type>(l_sqlite.insert(l_out)); \
      } else {                                                                                \
        l_out.id_ = l_storage.get(l_entt);                                                    \
        l_sqlite.update(l_out);                                                               \
      }                                                                                       \
    }                                                                                         \
  }
  DOODLE_SAVE_ID(project_helper, queue_project_helper_);
  DOODLE_SAVE_ID(scan_data_t, queue_scan_data_);

#undef DOODLE_SAVE_ID

#define DOODLE_SAVE_UUID(clas_, method_)          \
  {                                               \
    std::int32_t l_uuid{};                        \
    while ((method_).pop(l_uuid)) {               \
      l_sqlite.remove<clas_::database_t>(l_uuid); \
    }                                             \
  }

  DOODLE_SAVE_UUID(project_helper, queue_project_helper_uuid_);
  DOODLE_SAVE_UUID(scan_data_t, queue_scan_data_uuid_);
#undef DOODLE_SAVE_UUID

}  // namespace doodle

}  // namespace doodle