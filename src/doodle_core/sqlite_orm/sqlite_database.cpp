//
// Created by TD on 24-9-12.
//

#include "sqlite_database.h"

#include <doodle_core/core/app_base.h>

#include <sqlite_orm/sqlite_orm.h>
#include <sqlite_orm/std_filesystem_path_orm.h>
#include <sqlite_orm/uuid_to_blob.h>
namespace doodle {

namespace {
auto make_storage_doodle(const std::string& in_path) {
  using namespace sqlite_orm;

  return std::move(make_storage(
      in_path,  //
      make_index("scan_data_ue_uuid", &scan_data_t::database_t::ue_uuid_),
      make_index("scan_data_rig_uuid", &scan_data_t::database_t::rig_uuid_),
      make_index("scan_data_solve_uuid", &scan_data_t::database_t::solve_uuid_),
      make_table(
          "scan_data",  //
          make_column("id", &scan_data_t::database_t::id_, primary_key()),
          make_column("uuid_id", &scan_data_t::database_t::uuid_id_, unique()),
          make_column("ue_uuid", &scan_data_t::database_t::ue_uuid_, unique()),
          make_column("rig_uuid", &scan_data_t::database_t::rig_uuid_, unique()),
          make_column("solve_uuid", &scan_data_t::database_t::solve_uuid_, unique()),

          make_column("ue_path", &scan_data_t::database_t::ue_path_),
          make_column("rig_path", &scan_data_t::database_t::rig_path_),
          make_column("solve_path", &scan_data_t::database_t::solve_path_),

          make_column("project", &scan_data_t::database_t::project_id_),
          make_column("num", &scan_data_t::database_t::num_),  //
          make_column("name", &scan_data_t::database_t::name_),
          make_column("version", &scan_data_t::database_t::version_),
          foreign_key(&scan_data_t::database_t::project_id_).references(&project_helper::database_t::id_)
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
  ));
}
using sqlite_orm_type = decltype(make_storage_doodle(""));

auto get_cast_storage(const std::shared_ptr<void>& in_storage) {
  return std::static_pointer_cast<sqlite_orm_type>(in_storage);
}
}  // namespace

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

std::vector<scan_data_t::database_t> sqlite_database::find_by_path_id(const uuid& in_id) {
  using namespace sqlite_orm;
  auto l_storage = get_cast_storage(storage_any_);
  return l_storage->get_all<scan_data_t::database_t>(sqlite_orm::where(
      sqlite_orm::c(&scan_data_t::database_t::ue_uuid_) == in_id ||
      sqlite_orm::c(&scan_data_t::database_t::rig_uuid_) == in_id || c(&scan_data_t::database_t::solve_uuid_) == in_id
  ));
}

template <>
boost::asio::awaitable<tl::expected<void, std::string>> sqlite_database::install(
    std::shared_ptr<scan_data_t::database_t> in_data
) {
  co_await boost::asio::post(boost::asio::bind_executor(*strand_, boost::asio::use_awaitable));
  auto l_storage = get_cast_storage(storage_any_);
  try {
    auto l_g = l_storage->transaction_guard();
    if (in_data->id_ == 0)
      in_data->id_ = l_storage->insert<scan_data_t::database_t>(*in_data);
    else {
      l_storage->replace<scan_data_t::database_t>(*in_data);
    }
    l_g.commit();
    co_return tl::expected<void, std::string>{};
  } catch (...) {
    co_return tl::expected<void, std::string>{tl::make_unexpected(boost::current_exception_diagnostic_information())};
  }
}

template <>
boost::asio::awaitable<tl::expected<void, std::string>> sqlite_database::install(
    std::shared_ptr<project_helper::database_t> in_data
) {
  co_await boost::asio::post(boost::asio::bind_executor(*strand_, boost::asio::use_awaitable));

  try {
    auto l_storage = get_cast_storage(storage_any_);
    auto l_g       = l_storage->transaction_guard();

    if (in_data->id_ == 0)
      in_data->id_ = l_storage->insert<project_helper::database_t>(*in_data);
    else {
      l_storage->replace<project_helper::database_t>(*in_data);
    }
    l_g.commit();
    co_return tl::expected<void, std::string>{};
  } catch (...) {
    co_return tl::expected<void, std::string>{tl::make_unexpected(boost::current_exception_diagnostic_information())};
  }
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

  co_await boost::asio::post(boost::asio::bind_executor(*strand_, boost::asio::use_awaitable));
  try {
    auto l_storage = get_cast_storage(storage_any_);

    std::size_t l_split =
        std::distance(in_data->begin(), std::ranges::find_if(*in_data, [](const scan_data_t::database_t& in_) {
                        return in_.id_ != 0;
                      }));

    {
      // 步进大小
      constexpr std::size_t g_step_size{5000};
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
    }

    for (std::size_t i = 0; i < l_split; ++i) {
      using namespace sqlite_orm;
      auto l_v = l_storage->select(
          &scan_data_t::database_t::id_,
          sqlite_orm::where(c(&scan_data_t::database_t::uuid_id_) == (*in_data)[i].uuid_id_)
      );
      if (!l_v.empty()) (*in_data)[i].id_ = l_v.front();
    }
    co_return tl::expected<void, std::string>{};
  } catch (...) {
    co_return tl::expected<void, std::string>{tl::make_unexpected(boost::current_exception_diagnostic_information())};
  }
}

void sqlite_database::load(const FSys::path& in_path) { set_path(in_path); }

}  // namespace doodle