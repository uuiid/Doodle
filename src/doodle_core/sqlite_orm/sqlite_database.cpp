//
// Created by TD on 24-9-12.
//

#include "sqlite_database.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_xlsx_task_info.h>
#include <doodle_core/sqlite_orm/detail/assets_type_enum.h>
#include <doodle_core/sqlite_orm/detail/attendance_enum.h>
#include <doodle_core/sqlite_orm/detail/macro.h>
#include <doodle_core/sqlite_orm/detail/std_chrono_duration.h>
#include <doodle_core/sqlite_orm/detail/std_chrono_time_point.h>
#include <doodle_core/sqlite_orm/detail/std_chrono_zoned_time.h>
#include <doodle_core/sqlite_orm/detail/std_filesystem_path_orm.h>
#include <doodle_core/sqlite_orm/detail/uuid_to_blob.h>

#include "metadata/attendance.h"
#include <sqlite_orm/sqlite_orm.h>
namespace doodle {

namespace {
auto make_storage_doodle(const std::string& in_path) {
  using namespace sqlite_orm;

  return std::move(make_storage(
      in_path,  //
      make_index("assets_file_tab_uuid_id_index", &assets_file_helper::database_t::uuid_id_),
      make_table(
          "assets_file_tab",  //
          make_column("id", &assets_file_helper::database_t::id_, primary_key()),
          make_column("uuid_id", &assets_file_helper::database_t::uuid_id_, unique()),
          make_column("label", &assets_file_helper::database_t::label_),
          make_column("parent_uuid", &assets_file_helper::database_t::uuid_parent_),
          make_column("path", &assets_file_helper::database_t::path_),
          make_column("notes", &assets_file_helper::database_t::notes_),
          make_column("active", &assets_file_helper::database_t::active_),
          make_column("parent_id", &assets_file_helper::database_t::parent_id_),
          foreign_key(&assets_file_helper::database_t::parent_id_).references(&assets_helper::database_t::id_)
      ),

      make_index("assets_tab_uuid_id_index", &assets_helper::database_t::uuid_id_),
      make_table(
          "assets_tab",  //
          make_column("id", &assets_helper::database_t::id_, primary_key()),
          make_column("uuid_id", &assets_helper::database_t::uuid_id_, unique()),
          make_column("label", &assets_helper::database_t::label_),
          make_column("parent_uuid", &assets_helper::database_t::uuid_parent_)
      ),
      make_index("kitsu_task_type_tab_uuid_id_index", &metadata::kitsu::task_type_t::uuid_id_),
      make_index("kitsu_task_type_tab_kitsu_uuid_index", &metadata::kitsu::task_type_t::kitsu_uuid_),
      make_table(
          "kitsu_task_type_tab",                                                 //
          make_column("id", &metadata::kitsu::task_type_t::id_, primary_key()),  //
          make_column("uuid_id", &metadata::kitsu::task_type_t::uuid_id_, unique()),
          make_column("kitsu_uuid", &metadata::kitsu::task_type_t::kitsu_uuid_),
          make_column("name", &metadata::kitsu::task_type_t::name_),
          make_column("use_chick_files", &metadata::kitsu::task_type_t::use_chick_files)
      ),
      make_index("attendance_tab_uuid_id_index", &attendance_helper::database_t::uuid_id_),
      make_index("attendance_tab_create_date_index", &attendance_helper::database_t::create_date_),
      make_table(
          "attendance_tab",                                                       //
          make_column("id", &attendance_helper::database_t::id_, primary_key()),  //
          make_column("uuid_id", &attendance_helper::database_t::uuid_id_, unique()),
          make_column("start_time", &attendance_helper::database_t::start_time_),
          make_column("end_time", &attendance_helper::database_t::end_time_),
          make_column("remark", &attendance_helper::database_t::remark_),
          make_column("att_enum", &attendance_helper::database_t::type_),
          make_column("create_date", &attendance_helper::database_t::create_date_),
          make_column("update_time", &attendance_helper::database_t::update_time_),
          make_column("dingding_id", &attendance_helper::database_t::dingding_id_),
          make_column("user_id", &attendance_helper::database_t::user_ref),
          foreign_key(&attendance_helper::database_t::user_ref).references(&user_helper::database_t::id_)
      ),

      make_index("work_xlsx_task_info_tab_year_month_index", &work_xlsx_task_info_helper::database_t::year_month_),
      make_index("work_xlsx_task_info_tab_user_index", &work_xlsx_task_info_helper::database_t::user_ref_),
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
          make_column("user_id", &work_xlsx_task_info_helper::database_t::user_ref_),
          make_column("kitsu_task_ref_id", &work_xlsx_task_info_helper::database_t::kitsu_task_ref_id_),
          foreign_key(&work_xlsx_task_info_helper::database_t::user_ref_).references(&user_helper::database_t::id_)
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

      make_index("project_tab_uuid", &project_helper::database_t::uuid_id_),
      make_index("project_tab_kitsu_uuid", &project_helper::database_t::kitsu_uuid_),
      make_table(
          "project_tab",                                                       //
          make_column("id", &project_helper::database_t::id_, primary_key()),  //
          make_column("uuid_id", &project_helper::database_t::uuid_id_, unique()),
          make_column("name", &project_helper::database_t::name_),  //
          make_column("path", &project_helper::database_t::path_),
          make_column("en_str", &project_helper::database_t::en_str_),  //
          make_column("shor_str", &project_helper::database_t::shor_str_),
          make_column("local_path", &project_helper::database_t::local_path_),
          make_column("auto_upload_path", &project_helper::database_t::auto_upload_path_),
          make_column("kitsu_uuid", &project_helper::database_t::kitsu_uuid_)
      )
  ));
}
using sqlite_orm_type = decltype(make_storage_doodle(""));

constexpr std::size_t g_step_size{500};
}  // namespace
struct sqlite_database_impl {
  using executor_type   = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;
  using strand_type     = boost::asio::strand<boost::asio::io_context::executor_type>;
  using strand_type_ptr = std::shared_ptr<strand_type>;
  strand_type strand_;
  sqlite_orm_type storage_any_;

  explicit sqlite_database_impl(const FSys::path& in_path)
      : strand_(boost::asio::make_strand(g_io_context())),
        storage_any_(std::move(make_storage_doodle(in_path.generic_string()))) {
    storage_any_.open_forever();
    storage_any_.sync_schema(true);
    default_logger_raw()->info("sql thread safe {} ", sqlite_orm::threadsafe());
  }

  std::vector<project_helper::database_t> find_project_by_name(const std::string& in_name) {
    using namespace sqlite_orm;
    return storage_any_.get_all<project_helper::database_t>(
        sqlite_orm::where(sqlite_orm::c(&project_helper::database_t::name_) == in_name)
    );
  }
  std::vector<attendance_helper::database_t> get_attendance(
      const std::int64_t& in_ref_id, const chrono::local_days& in_data
  ) {
    using namespace sqlite_orm;

    return storage_any_.get_all<attendance_helper::database_t>(where(
        c(&attendance_helper::database_t::user_ref) == in_ref_id &&
        c(&attendance_helper::database_t::create_date_) == in_data
    ));
  }

  std::vector<attendance_helper::database_t> get_attendance(
      const std::int64_t& in_ref_id, const std::vector<chrono::local_days>& in_data
  ) {
    using namespace sqlite_orm;
    return storage_any_.get_all<attendance_helper::database_t>(where(
        c(&attendance_helper::database_t::user_ref) == in_ref_id &&
        in(&attendance_helper::database_t::create_date_, in_data)
    ));
  }
  std::vector<work_xlsx_task_info_helper::database_t> get_work_xlsx_task_info(
      const std::int64_t& in_ref_id, const chrono::local_days& in_data
  ) {
    using namespace sqlite_orm;
    return storage_any_.get_all<work_xlsx_task_info_helper::database_t>(where(
        c(&work_xlsx_task_info_helper::database_t::user_ref_) == in_ref_id &&
        c(&work_xlsx_task_info_helper::database_t::year_month_) == in_data
    ));
  }
#define DOODLE_TO_SQLITE_THREAD()                                 \
  auto this_executor = co_await boost::asio::this_coro::executor; \
  co_await boost::asio::post(boost::asio::bind_executor(strand_, boost::asio::use_awaitable));

  template <typename T>
  std::vector<T> get_all() {
    using namespace sqlite_orm;
    return storage_any_.get_all<T>();
  }

  template <typename T>
  std::vector<T> get_all(const uuid& in_uuid) {
    using namespace sqlite_orm;
    return storage_any_.get_all<T>(sqlite_orm::where(sqlite_orm::c(&T::kitsu_uuid_) == in_uuid));
  }

  template <typename T>
  std::int64_t uuid_to_id(const uuid& in_uuid) {
    using namespace sqlite_orm;
    auto l_v = storage_any_.select(&T::id_, sqlite_orm::where(sqlite_orm::c(&T::uuid_id_) == in_uuid));
    return l_v.empty() ? 0 : l_v[0];
  }

  template <typename T>
  std::vector<T> get_by_uuid(const uuid& in_uuid) {
    using namespace sqlite_orm;
    return storage_any_.get_all<T>(sqlite_orm::where(sqlite_orm::c(&T::uuid_id_) == in_uuid));
  }
  template <typename T>
  std::vector<T> get_by_kitsu_uuid(const uuid& in_uuid) {
    using namespace sqlite_orm;
    return storage_any_.get_all<T>(sqlite_orm::where(sqlite_orm::c(&T::kitsu_uuid_) == in_uuid));
  }

  template <typename T>
  boost::asio::awaitable<tl::expected<void, std::string>> install(std::shared_ptr<T> in_data) {
    DOODLE_TO_SQLITE_THREAD();
    tl::expected<void, std::string> l_ret{};
    try {
      auto l_g = storage_any_.transaction_guard();
      if (in_data->id_ == 0)
        in_data->id_ = storage_any_.insert<T>(*in_data);
      else {
        storage_any_.replace<T>(*in_data);
      }
      l_g.commit();
    } catch (...) {
      l_ret = tl::make_unexpected(boost::current_exception_diagnostic_information());
    }
    DOODLE_TO_SELF();
    co_return l_ret;
  }
  template <typename T>
  boost::asio::awaitable<tl::expected<void, std::string>> install_range(std::shared_ptr<std::vector<T>> in_data) {
    if (!std::is_sorted(in_data->begin(), in_data->end(), [](const auto& in_r, const auto& in_l) {
          return in_r.id_ < in_l.id_;
        }))
      std::sort(in_data->begin(), in_data->end(), [](const auto& in_r, const auto& in_l) {
        return in_r.id_ < in_l.id_;
      });
    std::size_t l_split =
        std::distance(in_data->begin(), std::ranges::find_if(*in_data, [](const auto& in_) { return in_.id_ != 0; }));

    DOODLE_TO_SQLITE_THREAD();
    tl::expected<void, std::string> l_ret{};
    try {
      auto l_g = storage_any_.transaction_guard();
      for (std::size_t i = 0; i < l_split;) {
        auto l_end = std::min(i + g_step_size, l_split);
        storage_any_.insert_range<T>(in_data->begin() + i, in_data->begin() + l_end);
        i = l_end;
      }

      for (std::size_t i = l_split; i < in_data->size();) {
        auto l_end = std::min(i + g_step_size, in_data->size());
        storage_any_.replace_range<T>(in_data->begin() + i, in_data->begin() + l_end);
        i = l_end;
      }
      l_g.commit();

      for (std::size_t i = 0; i < l_split; ++i) {
        using namespace sqlite_orm;
        auto l_v = storage_any_.select(&T::id_, sqlite_orm::where(c(&T::uuid_id_) == (*in_data)[i].uuid_id_));
        if (!l_v.empty()) (*in_data)[i].id_ = l_v.front();
      }

    } catch (...) {
      l_ret = tl::make_unexpected(boost::current_exception_diagnostic_information());
    }
    DOODLE_TO_SELF();
    co_return l_ret;
  }

  template <typename T>
  boost::asio::awaitable<tl::expected<void, std::string>> remove(std::shared_ptr<std::vector<std::int64_t>> in_data) {
    DOODLE_TO_SQLITE_THREAD();
    tl::expected<void, std::string> l_ret{};

    try {
      auto l_g = storage_any_.transaction_guard();
      storage_any_.remove_all<T>(sqlite_orm::where(sqlite_orm::in(&T::id_, *in_data)));
      l_g.commit();
    } catch (...) {
      l_ret = tl::make_unexpected(boost::current_exception_diagnostic_information());
    }
    DOODLE_TO_SELF();
    co_return l_ret;
  }
  template <typename T>
  boost::asio::awaitable<tl::expected<void, std::string>> remove(std::shared_ptr<uuid> in_data) {
    DOODLE_TO_SQLITE_THREAD();
    tl::expected<void, std::string> l_ret{};

    try {
      auto l_g = storage_any_.transaction_guard();
      storage_any_.remove_all<T>(sqlite_orm::where(sqlite_orm::c(&T::uuid_id_) = *in_data));
      l_g.commit();
    } catch (...) {
      l_ret = tl::make_unexpected(boost::current_exception_diagnostic_information());
    }
    DOODLE_TO_SELF();
    co_return l_ret;
  }

  template <typename T>
  std::vector<T> get_by_parent_id(const uuid& in_id) {
    using namespace sqlite_orm;
    return storage_any_.get_all<T>(sqlite_orm::where(sqlite_orm::c(&T::uuid_parent_) == in_id));
  }
};

void sqlite_database::load(const FSys::path& in_path) { impl_ = std::make_shared<sqlite_database_impl>(in_path); }

std::vector<project_helper::database_t> sqlite_database::find_project_by_name(const std::string& in_name) {
  return impl_->find_project_by_name(in_name);
}

std::vector<attendance_helper::database_t> sqlite_database::get_attendance(
    const std::int64_t& in_ref_id, const chrono::local_days& in_data
) {
  return impl_->get_attendance(in_ref_id, in_data);
}
std::vector<attendance_helper::database_t> sqlite_database::get_attendance(
    const std::int64_t& in_ref_id, const std::vector<chrono::local_days>& in_data
) {
  return impl_->get_attendance(in_ref_id, in_data);
}
std::vector<work_xlsx_task_info_helper::database_t> sqlite_database::get_work_xlsx_task_info(
    const std::int64_t& in_ref_id, const chrono::local_days& in_data
) {
  return impl_->get_work_xlsx_task_info(in_ref_id, in_data);
}

DOODLE_GET_BY_PARENT_ID_SQL(assets_file_helper::database_t);
DOODLE_GET_BY_PARENT_ID_SQL(assets_helper::database_t);

DOODLE_UUID_TO_ID(project_helper::database_t)
DOODLE_UUID_TO_ID(user_helper::database_t)
DOODLE_UUID_TO_ID(metadata::kitsu::task_type_t)
DOODLE_UUID_TO_ID(assets_file_helper::database_t)
DOODLE_UUID_TO_ID(assets_helper::database_t)

DOODLE_GET_BY_KITSU_UUID_SQL(project_helper::database_t)
DOODLE_GET_BY_KITSU_UUID_SQL(metadata::kitsu::task_type_t)

DOODLE_GET_BY_UUID_SQL(user_helper::database_t)
DOODLE_GET_BY_UUID_SQL(work_xlsx_task_info_helper::database_t)
DOODLE_GET_BY_UUID_SQL(assets_file_helper::database_t)
DOODLE_GET_BY_UUID_SQL(assets_helper::database_t)

DOODLE_GET_ALL_SQL(project_helper::database_t)
DOODLE_GET_ALL_SQL(user_helper::database_t)
DOODLE_GET_ALL_SQL(metadata::kitsu::task_type_t)
DOODLE_GET_ALL_SQL(assets_file_helper::database_t)
DOODLE_GET_ALL_SQL(assets_helper::database_t)

DOODLE_INSTALL_SQL(project_helper::database_t)
DOODLE_INSTALL_SQL(user_helper::database_t)
DOODLE_INSTALL_SQL(metadata::kitsu::task_type_t)
DOODLE_INSTALL_SQL(assets_file_helper::database_t)
DOODLE_INSTALL_SQL(assets_helper::database_t)

DOODLE_INSTALL_RANGE(project_helper::database_t)
DOODLE_INSTALL_RANGE(attendance_helper::database_t)
DOODLE_INSTALL_RANGE(work_xlsx_task_info_helper::database_t)
DOODLE_INSTALL_RANGE(metadata::kitsu::task_type_t)

DOODLE_REMOVE_RANGE(attendance_helper::database_t)
DOODLE_REMOVE_RANGE(work_xlsx_task_info_helper::database_t)
DOODLE_REMOVE_RANGE(metadata::kitsu::task_type_t)
DOODLE_REMOVE_RANGE(assets_file_helper::database_t)
DOODLE_REMOVE_RANGE(assets_helper::database_t)

DOODLE_REMOVE_BY_UUID(assets_helper::database_t)
DOODLE_REMOVE_BY_UUID(assets_file_helper::database_t)

}  // namespace doodle