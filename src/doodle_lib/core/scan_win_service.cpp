//
// Created by TD on 2023/12/26.
//

#include "scan_win_service.h"

#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/core/program_info.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/scan_data_t.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_app/app/app_command.h>

#include <doodle_lib/core/scan_assets/character_scan_category.h>
#include <doodle_lib/core/scan_assets/prop_scan_category.h>
#include <doodle_lib/core/scan_assets/scan_category_service.h>
#include <doodle_lib/core/scan_assets/scene_scan_category.h>

#include <boost/asio/experimental/parallel_group.hpp>

#include <tl/expected.hpp>
#include <wil/com.h>
namespace doodle {

namespace {
template <typename CompletionHandler>

auto to_scan_data(
    boost::asio::thread_pool& in_pool, const std::shared_ptr<project_helper::database_t>& in_project_root,
    const std::shared_ptr<details::scan_category_t>& in_scan_category_ptr, CompletionHandler&& in_completion
) {
  using expected_t              = tl::expected<std::vector<details::scan_category_data_ptr>, std::string>;
  in_scan_category_ptr->logger_ = spdlog::default_logger();
  return boost::asio::async_initiate<CompletionHandler, void(expected_t)>(
      [&in_project_root, &in_scan_category_ptr, &in_pool](auto&& in_completion_handler) {
        auto l_f = std::make_shared<std::decay_t<decltype(in_completion_handler)>>(
            std::forward<decltype(in_completion_handler)>(in_completion_handler)
        );
        boost::asio::post(in_pool, [in_project_root, in_scan_category_ptr, l_f]() {
          expected_t l_expected{};
          try {
            std::vector<details::scan_category_data_ptr> l_list = in_scan_category_ptr->scan(in_project_root);
            // for (auto&& l_ : l_list) {
            //   in_scan_category_ptr->scan_file_hash(l_);
            // }
            l_expected                                          = std::move(l_list);
          } catch (...) {
            l_expected = tl::make_unexpected(boost::current_exception_diagnostic_information());
          }

          boost::asio::post(boost::asio::prepend(std::move(*l_f), l_expected));
        });
      },
      in_completion
  );
};
void scan_win_service_id_is_nil(boost::uuids::uuid& in_uuid, const FSys::path& in_path) {
  if (in_uuid.is_nil() && FSys::exists(in_path)) {
    in_uuid = core_set::get_set().get_uuid();
    for (auto i = 0; i < 3; ++i) {
      try {
        FSys::software_flag_file(in_path, in_uuid);
        break;
      } catch (const wil::ResultException& in) {
        default_logger_raw()->error("生成uuid失败 {}", in.what());
      }
    }
  }
}
}  // namespace

void scan_win_service_t::start() {
  executor_ = boost::asio::make_strand(g_io_context());
  timer_    = std::make_shared<timer_t>(executor_);
  logger_   = std::make_shared<spdlog::logger>("scan_category");
  logger_->sinks().emplace_back(g_logger_ctrl().rotating_file_sink_);

  boost::asio::co_spawn(
      executor_, begin_scan(),
      boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached)
  );
}

boost::asio::awaitable<void> scan_win_service_t::begin_scan() {
  auto l_prjs = g_ctx().get<sqlite_database>().get_all<project_helper::database_t>();
  project_roots_.reserve(l_prjs.size());
  for (auto&& l_prj : l_prjs) {
    project_roots_.emplace_back(std::make_shared<project_helper::database_t>(std::move(l_prj)));
  }
  scan_categories_ = {
      std::make_shared<details::character_scan_category_t>(), std::make_shared<details::scene_scan_category_t>(),
      std::make_shared<details::prop_scan_category_t>()
  };
  for (auto&& i : scan_categories_) i->logger_ = logger_;

  create_project_map();
  std::vector<std::string> l_msg{};
  for (auto&& l_root : project_roots_)
    for (auto&& l_data : {
             "character",
             "scene",
             "prop",
         })
      l_msg.emplace_back(fmt::format("扫根目录瞄 {} 中 {} ", l_root->local_path_, l_data));

  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    // if (app_base::GetPtr()->is_stop()) co_return;
    using opt_t = decltype(to_scan_data(thread_pool_, project_roots_[0], scan_categories_[0], boost::asio::deferred));
    std::vector<opt_t> l_opts{};
    default_logger_raw()->log(log_loc(), level::info, "开始扫描");
    // 添加扫瞄操作
    for (auto&& l_root : project_roots_) {
      for (auto&& l_data : scan_categories_) {
        l_opts.emplace_back(to_scan_data(thread_pool_, l_root, l_data, boost::asio::deferred));
      }
    }

    auto [l_index, l_v] = co_await boost::asio::experimental::make_parallel_group(l_opts).async_wait(
        boost::asio::experimental::wait_for_all(),
        boost::asio::bind_executor(executor_, boost::asio::as_tuple(boost::asio::use_awaitable))
    );

    // 同步缓冲区
    std::int32_t l_current_index = !index_;
    scan_data_maps_[l_current_index].clear();
    scan_data_key_maps_[l_current_index].clear();
    for (auto i : l_index) {
      if (!l_v[i]) {
        default_logger_raw()->log(log_loc(), level::info, "扫描取消错误 {} {}", l_msg[i], l_v[i].error());
        continue;
      }
      add_handle(l_v[i].value(), l_current_index);
    }
    co_await seed_to_sql(l_current_index);
    // 交换缓冲区
    index_ = l_current_index;

    default_logger_raw()->log(log_loc(), level::info, "扫描完成");

    timer_->expires_after(30s);
    auto [l_ec] = co_await timer_->async_wait(boost::asio::as_tuple(boost::asio::use_awaitable));
    if (l_ec) {
      default_logger_raw()->log(log_loc(), level::info, "定时器取消 {}", l_ec.message());
      co_return;
    }
  }
}

void scan_win_service_t::create_project_map() {}

boost::asio::awaitable<void> scan_win_service_t::seed_to_sql(std::int32_t in_current_index) {
  auto& l_scan_key_data = scan_data_key_maps_[in_current_index];
  auto l_data           = g_ctx().get<sqlite_database>().get_all<scan_data_t::database_t>();
  std::unordered_map<scan::scan_key_t, std::size_t> l_old_map{};
  l_old_map.reserve(l_data.size());

  for (std::size_t i = 0; i < l_data.size(); ++i) {
    auto& l_d = l_data[i];
    l_old_map[{
        .dep_          = l_d.dep_,
        .season_       = season{l_d.season_},
        .project_      = l_d.project_,
        .number_       = l_d.num_ ? *l_d.num_ : std::string{},
        .name_         = l_d.name_,
        .version_name_ = l_d.version_ ? *l_d.version_ : std::string{},
    }]        = i;
  }

  // 开始查询更改
  auto l_install = std::make_shared<std::vector<scan_data_t::database_t>>();
  for (auto&& [l_key, l_val] : l_scan_key_data) {
    if (l_old_map.contains(l_key)) {
      auto& l_old = l_data[l_old_map[l_key]];
      if (l_old.ue_uuid_ != l_val->ue_file_.uuid_ || l_old.rig_uuid_ != l_val->rig_file_.uuid_ ||
          l_old.solve_uuid_ != l_val->solve_file_.uuid_ || l_old.hash_ != l_val->file_hash_) {
        auto&& l_new   = l_install->emplace_back(scan_data_t::database_t{static_cast<scan_data_t::database_t>(*l_val)});
        l_new.id_      = l_old.id_;
        l_new.uuid_id_ = l_old.uuid_id_;
      }
    } else {
      l_install->emplace_back(scan_data_t::database_t{static_cast<scan_data_t::database_t>(*l_val)}).uuid_id_ =
          core_set::get_set().get_uuid();
    }
  }

  // 此次开始删除旧的
  auto l_rem_ids = std::make_shared<std::vector<std::int64_t>>();
  {
    auto l_new_key = l_scan_key_data | ranges::views::keys | ranges::to<std::set<scan::scan_key_t>>();
    auto l_old_key = l_old_map | ranges::views::keys | ranges::to<std::set<scan::scan_key_t>>();
    {
      // ranges::;
      auto l_tmp = l_new_key;
      l_old_key.merge(l_tmp);
    }

    std::vector<scan::scan_key_t> l_set_rem{l_old_key.size() + l_new_key.size()};

    auto [old_orders_end, cut_orders_last] = std::ranges::set_difference(l_old_key, l_new_key, l_set_rem.begin());
    l_set_rem.erase(cut_orders_last, l_set_rem.end());
    l_rem_ids->reserve(l_set_rem.size());
    for (auto&& i : l_set_rem) {
      l_rem_ids->emplace_back(l_data[l_old_map[i]].id_);
    }
  }
  auto& l_data_base = g_ctx().get<sqlite_database>();
  if (auto l_r = co_await l_data_base.install_range<scan_data_t::database_t>(l_install); !l_r)
    default_logger_raw()->error(l_r.error());
  if (auto l_r = co_await l_data_base.remove<scan_data_t::database_t>(l_rem_ids); !l_r)
    default_logger_raw()->error(l_r.error());
}

void scan_win_service_t::add_handle(
    const std::vector<doodle::details::scan_category_data_ptr>& in_data_vec, std::int32_t in_current_index
) {
  auto& l_scan_data = scan_data_maps_[in_current_index];
  for (auto&& l_data : in_data_vec) {
    scan_win_service_id_is_nil(l_data->rig_file_.uuid_, l_data->rig_file_.path_);
    scan_win_service_id_is_nil(l_data->ue_file_.uuid_, l_data->ue_file_.path_);
    scan_win_service_id_is_nil(l_data->solve_file_.uuid_, l_data->solve_file_.path_);
    l_scan_data[l_data->rig_file_.uuid_]   = l_data;
    l_scan_data[l_data->ue_file_.uuid_]    = l_data;
    l_scan_data[l_data->solve_file_.uuid_] = l_data;
  }

  auto& l_scan_key_data = scan_data_key_maps_[in_current_index];
  for (auto&& l_data : in_data_vec) {
    l_scan_key_data[{
        .dep_          = l_data->assets_type_,
        .season_       = l_data->season_,
        .project_      = l_data->project_database_ptr->uuid_id_,
        .number_       = l_data->number_str_,
        .name_         = l_data->name_,
        .version_name_ = l_data->version_name_,
    }] = l_data;
  }
}
} // namespace doodle