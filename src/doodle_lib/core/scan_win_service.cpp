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
  timer_ = std::make_shared<timer_t>(g_io_context());
  begin_scan();
}

void scan_win_service_t::begin_scan() {
  project_roots_   = register_file_type::get_project_list();
  scan_categories_ = {
      std::make_shared<details::character_scan_category_t>(), std::make_shared<details::scene_scan_category_t>(),
      std::make_shared<details::prop_scan_category_t>()
  };
  if (!g_ctx().contains<details::scan_category_service_t>()) {
    g_ctx().emplace<details::scan_category_service_t>();
  }
  scan_categories_is_scan_.resize(scan_categories_.size() * project_roots_.size());

  if (app_base::GetPtr()->is_stop()) return;
  boost::asio::post(g_io_context(), [this]() { scan(); });
}

void scan_win_service_t::on_timer(const boost::system::error_code& ec) {
  if (ec) {
    default_logger_raw()->log(log_loc(), level::info, "定时器取消 {}", ec.message());
    return;
  }
  if (app_base::GetPtr()->is_stop()) return;
  begin_scan();
  //  add_handle();
}

void scan_win_service_t::scan() {
  scam_data_vec_.clear();
  for (auto&& l_data : scan_categories_is_scan_) {
    l_data = false;
  }
  std::size_t l_size{};
  for (auto&& l_root : project_roots_) {
    for (auto&& l_data : scan_categories_) {
      g_ctx().get<doodle::details::scan_category_service_t>().async_scan_files(
          l_root, l_data,
          boost::asio::bind_cancellation_slot(
              app_base::Get().on_cancel.slot(),
              boost::asio::bind_executor(
                  g_io_context(),
                  [l_i = l_size++, l_root = l_root.p_path, this](
                      std::vector<doodle::details::scan_category_data_ptr> in_vector, boost::system::error_code in_ec
                  ) {
                    scan_categories_is_scan_[l_i] = true;
                    if (in_ec) {
                      default_logger_raw()->log(log_loc(), level::err, "扫描资产失败:{} {}", in_ec.message(), l_root);
                      if (in_ec == boost::asio::error::operation_aborted) return;
                    }
                    add_handle(in_vector);
                  }
              )
          )
      );
    }
  }
}
void scan_win_service_t::add_handle(const std::vector<doodle::details::scan_category_data_ptr>& in_data_vec) {
  scam_data_vec_ |= ranges::actions::push_back(in_data_vec);

  static auto l_id_is_nil = [](boost::uuids::uuid& in_uuid, const FSys::path& in_path) {
    if (in_uuid.is_nil()) {
      in_uuid = core_set::get_set().get_uuid();
      FSys::software_flag_file(in_path, in_uuid);
    }
  };

  for (auto&& l_data : in_data_vec) {
    l_id_is_nil(l_data->rig_file_.uuid_, l_data->rig_file_.path_);
    l_id_is_nil(l_data->ue_file_.uuid_, l_data->ue_file_.path_);
    l_id_is_nil(l_data->solve_file_.uuid_, l_data->solve_file_.path_);
    scan_data_map_.insert({l_data->rig_file_.uuid_, l_data});
    scan_data_map_.insert({l_data->ue_file_.uuid_, l_data});
    scan_data_map_.insert({l_data->solve_file_.uuid_, l_data});
  }

  // 开始启动下一次循环
  if (app_base::GetPtr()->is_stop()) return;
  if (g_ctx().get<program_info>().stop_attr()) return;
  if (!std::all_of(scan_categories_is_scan_.begin(), scan_categories_is_scan_.end(), [](const bool& in_bool) {
        return in_bool;
      }))
    return;

  timer_->expires_after(std::chrono::seconds(30));
  timer_->async_wait(boost::asio::bind_cancellation_slot(
      app_base::Get().on_cancel.slot(), std::bind(&scan_win_service_t::on_timer, this, std::placeholders::_1)
  ));
}

}  // namespace doodle