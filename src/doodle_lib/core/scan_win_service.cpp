//
// Created by TD on 2023/12/26.
//

#include "scan_win_service.h"

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

#include <wil/com.h>

namespace doodle {
void scan_win_service_t::start() {
  executor_ = boost::asio::make_strand(g_io_context());
  timer_    = std::make_shared<timer_t>(executor_);

  boost::asio::co_spawn(
      executor_, begin_scan(),
      boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached)
  );
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
  create_project_map();
  std::vector<std::string> l_msg{};
  for (auto&& l_root : project_roots_)
    for (auto&& l_data : {
             "character",
             "scene",
             "prop",
         })
      l_msg.emplace_back(fmt::format("扫根目录瞄 {} 中 {} ", l_root.p_local_path, l_data));

  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    // if (app_base::GetPtr()->is_stop()) co_return;
    using opt_t = decltype(g_ctx().get<doodle::details::scan_category_service_t>().async_scan_files(
        project_roots_[0], scan_categories_[0], boost::asio::deferred
    ));
    std::vector<opt_t> l_opts{};
    default_logger_raw()->log(log_loc(), level::info, "开始扫描");
    // 添加扫瞄操作
    for (auto&& l_root : project_roots_) {
      for (auto&& l_data : scan_categories_) {
        l_opts.emplace_back(g_ctx().get<doodle::details::scan_category_service_t>().async_scan_files(
            l_root, l_data, boost::asio::deferred
        )

        );
      }
    }

    auto [l_index, l_v, l_ecs] = co_await boost::asio::experimental::make_parallel_group(l_opts).async_wait(
        boost::asio::experimental::wait_for_all(),
        boost::asio::bind_executor(executor_, boost::asio::as_tuple(boost::asio::use_awaitable))
    );

    // 同步缓冲区
    std::int32_t l_current_index         = !index_;
    scan_data_maps_[l_current_index]     = {};
    scan_data_key_maps_[l_current_index] = {};
    for (auto i : l_index) {
      if (l_ecs[i]) {
        default_logger_raw()->log(log_loc(), level::info, "扫描取消错误 {} {}", l_msg[i], l_ecs[i].message());
        continue;
      }
      add_handle(l_v[i], l_current_index);
    }
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

void scan_win_service_t::create_project_map() {
  for (auto&& l_root : project_roots_) {
    for (auto&& [l_e, p] : g_reg()->view<project>().each()) {
      if (p == l_root) {
        project_map_[l_root] = {*g_reg(), l_e};
      }
    }
  }
}

void scan_win_service_t::add_handle(
    const std::vector<doodle::details::scan_category_data_ptr>& in_data_vec, std::int32_t in_current_index
) {
  static auto l_id_is_nil = [](boost::uuids::uuid& in_uuid, const FSys::path& in_path) {
    if (in_uuid.is_nil()) {
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
  };
  auto& l_storage_ue      = g_reg()->storage<uuid>(detail::ue_path_id);
  auto& l_storage_rig     = g_reg()->storage<uuid>(detail::rig_path_id);
  auto& l_storage_solve   = g_reg()->storage<uuid>(detail::solve_path_id);
  auto& l_storage_project = g_reg()->storage<uuid>(detail::project_ref_id);
  std::map<uuid, entt::entity> l_uuid_entity_map{};

  for (auto&& [e, ue, rig, solve] : entt::basic_view{l_storage_ue, l_storage_rig, l_storage_solve}.each()) {
    l_uuid_entity_map[ue]    = e;
    l_uuid_entity_map[rig]   = e;
    l_uuid_entity_map[solve] = e;
  }

  for (auto&& l_data : in_data_vec) {
    l_id_is_nil(l_data->rig_file_.uuid_, l_data->rig_file_.path_);
    l_id_is_nil(l_data->ue_file_.uuid_, l_data->ue_file_.path_);
    l_id_is_nil(l_data->solve_file_.uuid_, l_data->solve_file_.path_);
    if (l_uuid_entity_map.contains(l_data->rig_file_.uuid_) || l_uuid_entity_map.contains(l_data->ue_file_.uuid_) ||
        l_uuid_entity_map.contains(l_data->solve_file_.uuid_)) {
      entt::entity l_e{};
      if (l_uuid_entity_map.contains(l_data->rig_file_.uuid_))
        l_e = l_uuid_entity_map[l_data->rig_file_.uuid_];
      else if (l_uuid_entity_map.contains(l_data->ue_file_.uuid_))
        l_e = l_uuid_entity_map[l_data->ue_file_.uuid_];
      else if (l_uuid_entity_map.contains(l_data->solve_file_.uuid_))
        l_e = l_uuid_entity_map[l_data->solve_file_.uuid_];

      scan_data_t l_scan_data{entt::handle{*g_reg(), l_e}};
      l_scan_data.ue_path(l_data->ue_file_.path_);
      l_scan_data.rig_path(l_data->rig_file_.path_);
      l_scan_data.solve_path(l_data->solve_file_.path_);

      l_scan_data.num_str(l_data->number_str_);
      l_scan_data.name(l_data->name_);
      l_scan_data.version(l_data->version_name_);
      l_scan_data.seed_to_sql();
    }
  }

  auto& l_scan_data = scan_data_maps_[in_current_index];
  for (auto&& l_data : in_data_vec) {
    l_id_is_nil(l_data->rig_file_.uuid_, l_data->rig_file_.path_);
    l_id_is_nil(l_data->ue_file_.uuid_, l_data->ue_file_.path_);
    l_id_is_nil(l_data->solve_file_.uuid_, l_data->solve_file_.path_);
    l_scan_data[l_data->rig_file_.uuid_]   = l_data;
    l_scan_data[l_data->ue_file_.uuid_]    = l_data;
    l_scan_data[l_data->solve_file_.uuid_] = l_data;
  }

  auto& l_scan_key_data = scan_data_key_maps_[in_current_index];
  for (auto&& l_data : in_data_vec) {
    l_scan_key_data[{
        .dep_          = l_data->assets_type_,
        .season_       = l_data->season_,
        .project_      = l_data->project_root_,
        .number_       = l_data->number_str_,
        .name_         = l_data->name_,
        .version_name_ = l_data->version_name_,
    }] = l_data;
  }
}
} // namespace doodle