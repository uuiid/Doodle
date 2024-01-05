//
// Created by TD on 2023/12/26.
//

#include "scan_win_service.h"

#include <doodle_core/core/program_info.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_app/app/app_command.h>

#include <doodle_lib/core/scan_assets/character_scan_category.h>
#include <doodle_lib/core/scan_assets/prop_scan_category.h>
#include <doodle_lib/core/scan_assets/scan_category_service.h>
#include <doodle_lib/core/scan_assets/scene_scan_category.h>

namespace doodle {

void scan_win_service_t::start() {
  timer_  = std::make_shared<timer_t>(g_io_context());
  signal_ = std::make_shared<signal_t>(g_io_context(), SIGINT, SIGTERM);
  signal_->async_wait(boost::asio::bind_cancellation_slot(
      app_base::Get().on_cancel.slot(),
      [](boost::system::error_code in_error_code, int in_sig) {
        default_logger_raw()->log(log_loc(), level::warn, "收到信号 {} {}", in_error_code.message(), in_sig);

        g_io_context().stop();
      }
  ));

  project_roots_   = register_file_type::get_project_list();
  scan_categories_ = {
      std::make_shared<details::character_scan_category_t>(), std::make_shared<details::scene_scan_category_t>(),
      std::make_shared<details::prop_scan_category_t>()
  };
  if (!g_ctx().contains<details::scan_category_service_t>()) {
    g_ctx().emplace<details::scan_category_service_t>();
  }
  scan_categories_is_scan_.resize(scan_categories_.size() * project_roots_.size());
  auto l_database_view = g_reg()->view<database>().each();

  handle_map_          = l_database_view | ranges::views::transform([](auto&& in_entity) {
                  return std::make_pair(
                      std::get<database&>(in_entity).uuid(), entt::handle{*g_reg(), std::get<entt::entity>(in_entity)}
                  );
                }) |
                ranges::to<std::map<uuid, entt::handle> >();

  timer_->expires_after(std::chrono::seconds(1));
  timer_->async_wait(boost::asio::bind_cancellation_slot(
      app_base::Get().on_cancel.slot(), std::bind(&scan_win_service_t::on_timer, this, std::placeholders::_1)
  ));
}

void scan_win_service_t::on_timer(const boost::system::error_code& ec) {
  if (ec) {
    default_logger_raw()->log(log_loc(), level::info, "定时器取消 {}", ec.message());
    return;
  }
  scan();
  //  add_handle();
}

void scan_win_service_t::scan() {
  scam_data_vec_.clear();
  std::ranges::for_each(scan_categories_is_scan_, [](auto&& in_bool) { in_bool = false; });
  std::size_t l_size{};
  for (auto&& l_root : project_roots_) {
    for (auto&& l_data : scan_categories_) {
      g_ctx().get<doodle::details::scan_category_service_t>().async_scan_files(
          l_root, l_data,
          boost::asio::bind_cancellation_slot(
              app_base::Get().on_cancel.slot(),
              boost::asio::bind_executor(
                  g_io_context(),
                  [l_i = l_size++, l_root = l_root.path_, this](
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
  for (auto l_data : in_data_vec) {
    if (!l_data) continue;
    auto l_handle_vec = l_data->create_handles(handle_map_, *g_reg());
    for (auto&& l_h : l_handle_vec) {
      if (l_h.any_of<database>()) handle_map_.emplace(l_h.get<database>().uuid(), l_h);
    }
  }
  // 开始启动下一次循环

  if (g_ctx().get<program_info>().stop_attr()) return;
  if (!std::all_of(scan_categories_is_scan_.begin(), scan_categories_is_scan_.end(), [](auto&& in_bool) {
        return in_bool;
      }))
    return;

  timer_->expires_after(std::chrono::seconds(30));
  timer_->async_wait(boost::asio::bind_cancellation_slot(
      app_base::Get().on_cancel.slot(), std::bind(&scan_win_service_t::on_timer, this, std::placeholders::_1)
  ));
}

}  // namespace doodle