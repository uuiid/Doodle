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

  auto l_storage = std::any_cast<sqlite_orm_type&>(storage_any_);

  {
    auto l_data = l_storage.get_all<project_helper::database_t>();
    project_helper::load_from_sql(*g_reg(), l_data);
  }

  {
    auto l_data = l_storage.get_all<scan_data_t::database_t>();
    scan_data_t::load_from_sql(*g_reg(), l_data);
  }
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
  auto& l_sqlite = std::any_cast<sqlite_orm_type&>(storage_any_);
  {
    std::vector<project_helper::database_t> l_out{queue_project_helper_.read_available()};
    while (queue_project_helper_.pop(l_out.emplace_back())) {
    }
    l_sqlite.insert_range(l_out.begin(), l_out.end());
  }

  {
    std::vector<scan_data_t::database_t> l_out{queue_scan_data_.read_available()};
    while (queue_scan_data_.pop(l_out.emplace_back())) {
    }
    l_sqlite.insert_range(l_out.begin(), l_out.end());
  }

  {
    std::int32_t l_uuid{};
    while (queue_project_helper_uuid_.pop(l_uuid)) {
      l_sqlite.remove<project_helper>(l_uuid);
    }
  }
  {
    std::int32_t l_uuid{};
    while (queue_scan_data_uuid_.pop(l_uuid)) {
      l_sqlite.remove<scan_data_t>(l_uuid);
    }
  }
}

}  // namespace doodle