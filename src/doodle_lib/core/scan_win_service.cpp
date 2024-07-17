//
// Created by TD on 2023/12/26.
//

#include "scan_win_service.h"

#include <doodle_core/core/program_info.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_app/app/app_command.h>

#include <doodle_lib/core/scan_assets/character_scan_category.h>
#include <doodle_lib/core/scan_assets/prop_scan_category.h>
#include <doodle_lib/core/scan_assets/scan_category_service.h>
#include <doodle_lib/core/scan_assets/scene_scan_category.h>

namespace doodle {
void scan_win_service_t::start() {
  executor_ = boost::asio::make_strand(g_io_context());
  timer_    = std::make_shared<timer_t>(executor_);

  boost::asio::co_spawn(executor_, begin_scan(),
                        boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached));
}

boost::asio::awaitable<void> scan_win_service_t::begin_scan() {
  project_roots_   = register_file_type::get_project_list();
  scan_categories_ = {
      std::make_shared<details::character_scan_category_t>(), std::make_shared<details::scene_scan_category_t>(),
      std::make_shared<details::prop_scan_category_t>()
  };
  if (!g_ctx().contains<details::scan_category_service_t>()) {
    g_ctx().emplace<details::scan_category_service_t>();
  }

  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    // if (app_base::GetPtr()->is_stop()) co_return;
    for (auto&& l_root : project_roots_) {
      for (auto&& l_data : scan_categories_) {
        auto [l_v, l_ec] = co_await g_ctx().get<doodle::details::scan_category_service_t>().async_scan_files(
          l_root, l_data, boost::asio::bind_executor(executor_, boost::asio::as_tuple(boost::asio::use_awaitable)));
        if (l_ec) {
          default_logger_raw()->log(log_loc(), level::err, "扫描资产失败:{} {}", l_ec.message(), l_root);
          if (l_ec == boost::asio::error::operation_aborted) co_return;
        }
        add_handle(l_v);
      }
    }

    timer_->expires_after(30s);
    auto [l_ec] = co_await timer_->async_wait(boost::asio::as_tuple(boost::asio::use_awaitable));
    if (l_ec) {
      default_logger_raw()->log(log_loc(), level::info, "定时器取消 {}", l_ec.message());
      co_return;
    }
  }
}


void scan_win_service_t::add_handle(const std::vector<doodle::details::scan_category_data_ptr>& in_data_vec) {
  static auto l_id_is_nil = [](boost::uuids::uuid& in_uuid, const FSys::path& in_path) {
    if (in_uuid.is_nil()) {
      in_uuid = core_set::get_set().get_uuid();
      FSys::software_flag_file(in_path, in_uuid);
    }
  };
  std::int32_t l_current_index     = !index_;
  scan_data_maps_[l_current_index] = scan_data_maps_[index_];
  auto& l_scan_data                = scan_data_maps_[l_current_index];

  for (auto&& l_data : in_data_vec) {
    l_id_is_nil(l_data->rig_file_.uuid_, l_data->rig_file_.path_);
    l_id_is_nil(l_data->ue_file_.uuid_, l_data->ue_file_.path_);
    l_id_is_nil(l_data->solve_file_.uuid_, l_data->solve_file_.path_);
    l_scan_data[l_data->rig_file_.uuid_]   = l_data;
    l_scan_data[l_data->ue_file_.uuid_]    = l_data;
    l_scan_data[l_data->solve_file_.uuid_] = l_data;
  }
  index_ = l_current_index;
}
} // namespace doodle