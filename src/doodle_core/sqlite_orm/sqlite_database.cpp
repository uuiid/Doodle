//
// Created by TD on 24-9-12.
//

#include "sqlite_database.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_xlsx_task_info.h>

#include <sqlite_orm/assets_type_enum.h>
#include <sqlite_orm/sqlite_orm.h>
#include <sqlite_orm/std_chrono_duration.h>
#include <sqlite_orm/std_chrono_time_point.h>
#include <sqlite_orm/std_chrono_zoned_time.h>
#include <sqlite_orm/std_filesystem_path_orm.h>
#include <sqlite_orm/uuid_to_blob.h>
namespace doodle {

namespace {
auto make_storage_doodle(const std::string& in_path) {
  using namespace sqlite_orm;
  return std::move(make_storage(
      in_path,  //
      make_index("work_xlsx_task_info_tab_year_month_index", &work_xlsx_task_info_helper::database_t::year_month_),
      make_index("work_xlsx_task_info_tab_user_index", &work_xlsx_task_info_helper::database_t::user_id_),
      make_table(
          "work_xlsx_task_info_tab",                                                       //
          make_column("id", &work_xlsx_task_info_helper::database_t::id_, primary_key()),  //
          make_column("uuid_id", &work_xlsx_task_info_helper::database_t::uuid_id_, unique()),
          make_column("start_time", &work_xlsx_task_info_helper::database_t::start_time_),
          make_column("end_time", &work_xlsx_task_info_helper::database_t::end_time_),
          make_column("duration", &work_xlsx_task_info_helper::database_t::duration_),
          make_column("remark", &work_xlsx_task_info_helper::database_t::remark_),
          make_column("user_remark", &work_xlsx_task_info_helper::database_t::user_remark_),
          make_column("year_month", &work_xlsx_task_info_helper::database_t::year_month_),
          make_column("user_id", &work_xlsx_task_info_helper::database_t::user_id_),
          make_column("kitsu_task_ref_id", &work_xlsx_task_info_helper::database_t::kitsu_task_ref_id_),
          foreign_key(&work_xlsx_task_info_helper::database_t::user_id_).references(&user_helper::database_t::id_)
      ),
      make_index("user_tab_dingding_index", &user_helper::database_t::dingding_id_),
      make_table(
          "user_tab",                                                       //
          make_column("id", &user_helper::database_t::id_, primary_key()),  //
          make_column("uuid_id", &user_helper::database_t::uuid_id_, unique()),
          make_column("mobile", &user_helper::database_t::mobile_),  //
          make_column("dingding_id", &user_helper::database_t::dingding_id_),
          make_column("dingding_company_id", &user_helper::database_t::dingding_company_id_)
      ),

      make_index("scan_data_ue_uuid", &scan_data_t::database_t::ue_uuid_),
      make_index("scan_data_rig_uuid", &scan_data_t::database_t::rig_uuid_),
      make_index("scan_data_solve_uuid", &scan_data_t::database_t::solve_uuid_),
      make_table(
          "scan_data",  //
          make_column("id", &scan_data_t::database_t::id_, primary_key()),
          make_column("uuid_id", &scan_data_t::database_t::uuid_id_, unique()),
          make_column("ue_uuid", &scan_data_t::database_t::ue_uuid_),
          make_column("rig_uuid", &scan_data_t::database_t::rig_uuid_),
          make_column("solve_uuid", &scan_data_t::database_t::solve_uuid_),

          make_column("ue_path", &scan_data_t::database_t::ue_path_),
          make_column("rig_path", &scan_data_t::database_t::rig_path_),
          make_column("solve_path", &scan_data_t::database_t::solve_path_),

          make_column("season", &scan_data_t::database_t::season_),
          make_column("assets_type_enum", &scan_data_t::database_t::dep_),

          make_column("project", &scan_data_t::database_t::project_id_),
          make_column("name", &scan_data_t::database_t::name_),
          make_column("version", &scan_data_t::database_t::version_),
          make_column("num", &scan_data_t::database_t::num_),  //
          // make_column("file_hash", &scan_data_t::database_t::hash_),
          foreign_key(&scan_data_t::database_t::project_id_).references(&project_helper::database_t::id_)
      ),  //
      make_table(
          "project_tab",                                                       //
          make_column("id", &project_helper::database_t::id_, primary_key()),  //
          make_column("uuid_id", &project_helper::database_t::uuid_id_, unique()),
          make_column("name", &project_helper::database_t::name_),  //
          make_column("path", &project_helper::database_t::path_),
          make_column("en_str", &project_helper::database_t::en_str_),  //
          make_column("shor_str", &project_helper::database_t::shor_str_),
          make_column("local_path", &project_helper::database_t::local_path_),
          make_column("auto_upload_path", &project_helper::database_t::auto_upload_path_)
      )
  ));
}
using sqlite_orm_type = decltype(make_storage_doodle(""));

auto get_cast_storage(const std::shared_ptr<void>& in_storage) {
  return std::static_pointer_cast<sqlite_orm_type>(in_storage);
}
}  // namespace

#define DOODLE_TO_SQLITE_THREAD()                                 \
  auto this_executor = co_await boost::asio::this_coro::executor; \
  co_await boost::asio::post(boost::asio::bind_executor(*strand_, boost::asio::use_awaitable));

void sqlite_database::set_path(const FSys::path& in_path) {
  strand_        = std::make_shared<strand_type>(boost::asio::make_strand(g_io_context()));
  storage_any_   = std::make_shared<sqlite_orm_type>(std::move(make_storage_doodle(in_path.generic_string())));
  auto l_storage = get_cast_storage(storage_any_);
  l_storage->open_forever();
  l_storage->sync_schema(true);
  default_logger_raw()->info("sql thread safe {} ", sqlite_orm::threadsafe());
}

template <>
std::vector<scan_data_t::database_t> sqlite_database::get_by_uuid<scan_data_t::database_t>(const uuid& in_uuid) {
  using namespace sqlite_orm;
  auto l_storage = get_cast_storage(storage_any_);
  return l_storage->get_all<scan_data_t::database_t>(
      sqlite_orm::where(sqlite_orm::c(&scan_data_t::database_t::uuid_id_) == in_uuid)
  );
}

template <>
std::vector<project_helper::database_t> sqlite_database::get_all() {
  auto l_storage = get_cast_storage(storage_any_);
  return l_storage->get_all<project_helper::database_t>();
}

template <>
std::vector<scan_data_t::database_t> sqlite_database::get_all() {
  auto l_storage = get_cast_storage(storage_any_);
  return l_storage->get_all<scan_data_t::database_t>();
}

template <>
std::vector<user_helper::database_t> sqlite_database::get_all() {
  auto l_storage = get_cast_storage(storage_any_);
  return l_storage->get_all<user_helper::database_t>();
}

std::vector<scan_data_t::database_t> sqlite_database::find_by_path_id(const uuid& in_id) {
  using namespace sqlite_orm;
  auto l_storage = get_cast_storage(storage_any_);
  return l_storage->get_all<scan_data_t::database_t>(sqlite_orm::where(
      sqlite_orm::c(&scan_data_t::database_t::ue_uuid_) == in_id ||
      sqlite_orm::c(&scan_data_t::database_t::rig_uuid_) == in_id || c(&scan_data_t::database_t::solve_uuid_) == in_id
  ));
}

std::vector<project_helper::database_t> sqlite_database::find_project_by_name(const std::string& in_name) {
  using namespace sqlite_orm;
  auto l_storage = get_cast_storage(storage_any_);
  return l_storage->get_all<project_helper::database_t>(
      sqlite_orm::where(sqlite_orm::c(&project_helper::database_t::name_) == in_name)
  );
}

template <>
boost::asio::awaitable<tl::expected<void, std::string>> sqlite_database::install(
    std::shared_ptr<scan_data_t::database_t> in_data
) {
  DOODLE_TO_SQLITE_THREAD();
  tl::expected<void, std::string> l_ret{};

  try {
    auto l_storage = get_cast_storage(storage_any_);
    auto l_g       = l_storage->transaction_guard();
    if (in_data->id_ == 0)
      in_data->id_ = l_storage->insert<scan_data_t::database_t>(*in_data);
    else {
      l_storage->replace<scan_data_t::database_t>(*in_data);
    }
    l_g.commit();
  } catch (...) {
    l_ret = tl::make_unexpected(boost::current_exception_diagnostic_information());
  }
  DOODLE_TO_SELF();
  co_return l_ret;
}

template <>
boost::asio::awaitable<tl::expected<void, std::string>> sqlite_database::install(
    std::shared_ptr<project_helper::database_t> in_data
) {
  DOODLE_TO_SQLITE_THREAD();
  tl::expected<void, std::string> l_ret{};

  try {
    auto l_storage = get_cast_storage(storage_any_);
    auto l_g       = l_storage->transaction_guard();

    if (in_data->id_ == 0)
      in_data->id_ = l_storage->insert<project_helper::database_t>(*in_data);
    else {
      l_storage->replace<project_helper::database_t>(*in_data);
    }
    l_g.commit();
  } catch (...) {
    l_ret = tl::make_unexpected(boost::current_exception_diagnostic_information());
  }
  DOODLE_TO_SELF();
  co_return l_ret;
}
template <>
boost::asio::awaitable<tl::expected<void, std::string>> sqlite_database::install(
    std::shared_ptr<user_helper::database_t> in_data
) {
  DOODLE_TO_SQLITE_THREAD();
  tl::expected<void, std::string> l_ret{};

  try {
    auto l_storage = get_cast_storage(storage_any_);
    auto l_g       = l_storage->transaction_guard();

    if (in_data->id_ == 0)
      in_data->id_ = l_storage->insert<user_helper::database_t>(*in_data);
    else {
      l_storage->replace<user_helper::database_t>(*in_data);
    }
    l_g.commit();
  } catch (...) {
    l_ret = tl::make_unexpected(boost::current_exception_diagnostic_information());
  }
  DOODLE_TO_SELF();
  co_return l_ret;
}

template <>
boost::asio::awaitable<tl::expected<void, std::string>> sqlite_database::install_range(
    std::shared_ptr<std::vector<scan_data_t::database_t>> in_data
) {
  if (!std::is_sorted(
          in_data->begin(), in_data->end(),
          [](scan_data_t::database_t& in_r, scan_data_t::database_t& in_l) { return in_r.id_ < in_l.id_; }
      ))
    std::sort(in_data->begin(), in_data->end(), [](scan_data_t::database_t& in_r, scan_data_t::database_t& in_l) {
      return in_r.id_ < in_l.id_;
    });
  std::size_t l_split = std::distance(
      in_data->begin(), std::ranges::find_if(*in_data, [](const scan_data_t::database_t& in_) { return in_.id_ != 0; })
  );

  DOODLE_TO_SQLITE_THREAD();
  tl::expected<void, std::string> l_ret{};

  try {
    auto l_storage = get_cast_storage(storage_any_);

    // 步进大小
    constexpr std::size_t g_step_size{500};
    auto l_g = l_storage->transaction_guard();
    // 每500次步进(插入步进)
    for (std::size_t i = 0; i < l_split;) {
      auto l_end = std::min(i + g_step_size, l_split);
      l_storage->insert_range<scan_data_t::database_t>(in_data->begin() + i, in_data->begin() + l_end);
      i = l_end;
    }
    // 替换步进
    for (std::size_t i = l_split; i < in_data->size();) {
      auto l_end = std::min(i + g_step_size, in_data->size());
      l_storage->replace_range<scan_data_t::database_t>(in_data->begin() + i, in_data->begin() + l_end);
      i = l_end;
    }
    l_g.commit();

    for (std::size_t i = 0; i < l_split; ++i) {
      using namespace sqlite_orm;
      auto l_v = l_storage->select(
          &scan_data_t::database_t::id_,
          sqlite_orm::where(c(&scan_data_t::database_t::uuid_id_) == (*in_data)[i].uuid_id_)
      );
      if (!l_v.empty()) (*in_data)[i].id_ = l_v.front();
    }

  } catch (...) {
    l_ret = tl::make_unexpected(boost::current_exception_diagnostic_information());
  }
  DOODLE_TO_SELF();
  co_return l_ret;
}

template <>
boost::asio::awaitable<tl::expected<void, std::string>> sqlite_database::remove<scan_data_t::database_t>(
    std::shared_ptr<std::vector<std::int64_t>> in_data
) {
  DOODLE_TO_SQLITE_THREAD();
  tl::expected<void, std::string> l_ret{};

  try {
    auto l_storage = get_cast_storage(storage_any_);
    auto l_g       = l_storage->transaction_guard();
    for (auto&& i : *in_data) {
      l_storage->remove<scan_data_t::database_t>(i);
    }
  } catch (...) {
    l_ret = tl::make_unexpected(boost::current_exception_diagnostic_information());
  }
  DOODLE_TO_SELF();
  co_return l_ret;
}

void sqlite_database::load(const FSys::path& in_path) { set_path(in_path); }

}  // namespace doodle