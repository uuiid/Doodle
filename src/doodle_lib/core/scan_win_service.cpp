//
// Created by TD on 2023/12/26.
//

#include "scan_win_service.h"

#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/core/app_base.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/scan_data_t.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_lib/core/scan_assets/character_scan_category.h>
#include <doodle_lib/core/scan_assets/prop_scan_category.h>
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
  using expected_t = tl::expected<std::vector<details::scan_category_data_ptr>, std::string>;
  return boost::asio::async_initiate<CompletionHandler, void(expected_t)>(
      [&in_project_root, &in_scan_category_ptr, &in_pool](auto&& in_completion_handler) {
        auto l_f = std::make_shared<std::decay_t<decltype(in_completion_handler)>>(
            std::forward<decltype(in_completion_handler)>(in_completion_handler)
        );
        in_scan_category_ptr->cancellation_state_ =
            std::make_shared<boost::asio::cancellation_state>(boost ::asio::get_associated_cancellation_slot(*l_f));
        boost::asio::post(in_pool, [in_project_root, in_scan_category_ptr, l_f]() {
          expected_t l_expected{};
          try {
            std::vector<details::scan_category_data_ptr> l_list = in_scan_category_ptr->scan(in_project_root);
            // for (auto&& l_ : l_list) {
            //   in_scan_category_ptr->scan_file_hash(l_);
            // }
            if (in_scan_category_ptr->cancellation_state_ &&
                in_scan_category_ptr->cancellation_state_->cancelled() != boost::asio::cancellation_type::none)
              l_expected = tl::make_unexpected("用户取消"s);
            else
              l_expected = std::move(l_list);
          } catch (...) {
            l_expected = tl::make_unexpected(
                fmt::format(
                    "项目 {} 路径{} 错误 {}", in_project_root->name_, in_project_root->path_,
                    boost::current_exception_diagnostic_information()
                )
            );
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
  logger_   = std::make_shared<spdlog::async_logger>(
      "scan_category", g_logger_ctrl().make_file_sink_mt("scan_win_service"), spdlog::thread_pool()
  );
  logger_->set_level(level::debug);

  boost::asio::co_spawn(
      executor_, begin_scan(),
      boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached)
  );
}
void scan_win_service_t::init_all_map() {
  auto l_path = core_set::get_set().get_cache_root() / jaon_file_name_;
  if (!FSys::exists(l_path) || FSys::file_size(l_path) == 0) return;

  create_project();
  std::map<uuid, std::shared_ptr<project_helper::database_t>> l_pro_map{};
  for (auto&& i : project_roots_) {
    l_pro_map.emplace(i->uuid_id_, i);
  }
  // std::vector<doodle::details::scan_category_data_ptr> l_data_vec{};
  // try {
  //   nlohmann::json l_json =
  //       nlohmann::json::parse(FSys::ifstream{core_set::get_set().get_cache_root() / jaon_file_name_});
  //   for (auto&& l_v : l_json) {
  //     if (auto l_uuid = l_v["project_database_ptr"].get<uuid>(); l_pro_map.contains(l_uuid))
  //       l_data_vec
  //           .emplace_back(std::make_shared<details::scan_category_data_t>(l_v.get<details::scan_category_data_t>()))
  //           ->project_database_ptr = l_pro_map.at(l_uuid);
  //   }
  // } catch (...) {
  //   default_logger_raw()->error("加载扫描数据失败 {}", boost::current_exception_diagnostic_information());
  // }
  // add_handle(l_data_vec, index_);
}

boost::asio::awaitable<void> scan_win_service_t::begin_scan() {
  scan_categories_ = {
      std::make_shared<details::character_scan_category_t>(), std::make_shared<details::scene_scan_category_t>(),
      std::make_shared<details::prop_scan_category_t>()
  };
  for (auto&& i : scan_categories_) i->logger_ = logger_;
  init_all_map();

  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    create_project();
    // if (app_base::GetPtr()->is_stop()) co_return;
    using opt_t = decltype(to_scan_data(thread_pool_, project_roots_[0], scan_categories_[0], boost::asio::deferred));
    std::vector<opt_t> l_opts{};
    logger_->log(log_loc(), level::info, "开始扫描");
    // 添加扫瞄操作
    for (auto&& l_root : project_roots_) {
      for (auto&& l_data : scan_categories_) {
        l_opts.emplace_back(to_scan_data(thread_pool_, l_root, l_data, boost::asio::deferred));
      }
    }
    if (!l_opts.empty()) {
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
          logger_->debug(l_v[i].error());
          continue;
        }
        add_handle(l_v[i].value(), l_current_index);
      }
      seed_to_sql(l_current_index);
      // 交换缓冲区
      index_ = l_current_index;
    }

    logger_->log(log_loc(), level::info, "扫描完成");

    timer_->expires_after(30s);
    auto [l_ec] = co_await timer_->async_wait(boost::asio::as_tuple(boost::asio::use_awaitable));
    if (l_ec) {
      logger_->log(log_loc(), level::info, "定时器取消 {}", l_ec.message());
      co_return;
    }
  }
}

void scan_win_service_t::create_project() {
  project_roots_.clear();
  auto l_prjs = g_ctx().get<sqlite_database>().get_all<project_helper::database_t>();
  project_roots_.reserve(l_prjs.size());
  for (auto&& l_prj : l_prjs) {
    project_roots_.emplace_back(std::make_shared<project_helper::database_t>(std::move(l_prj)));
  }
}

void scan_win_service_t::seed_to_sql(std::int32_t in_current_index) {
  if (!use_cache_) return;
  nlohmann::json l_json{};

  auto& l_scan_data = scan_data_maps_[in_current_index];
  for (auto&& [l_k, l_v] : l_scan_data) {
    l_json.emplace_back(*l_v);
  }
  FSys::ofstream{core_set::get_set().get_cache_root() / jaon_file_name_} << l_json.dump();
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
        .project_      = l_data->project_database_ptr ? l_data->project_database_ptr->uuid_id_ : uuid{},
        .number_       = l_data->number_str_,
        .name_         = l_data->name_,
        .version_name_ = l_data->version_name_,
    }] = l_data;
  }
}
}  // namespace doodle