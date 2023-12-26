//
// Created by TD on 2023/12/26.
//

#include "scan_win_service.h"

#include <doodle_core/core/program_info.h>
#include <doodle_core/metadata/metadata.h>

#include <doodle_app/app/app_command.h>

#include <doodle_lib/core/scan_assets/character_scan_category.h>
#include <doodle_lib/core/scan_assets/prop_scan_category.h>
#include <doodle_lib/core/scan_assets/scan_category_service.h>
#include <doodle_lib/core/scan_assets/scene_scan_category.h>

#include <wil/resource.h>
#include <wil/result.h>
#include <windows.h>
namespace doodle {

namespace {

void install_scan_win_service() {
  DWORD l_size{};
  l_size = ::GetModuleFileNameW(nullptr, nullptr, l_size);
  THROW_LAST_ERROR_IF(::GetLastError() != ERROR_INSUFFICIENT_BUFFER);

  std::wstring l_path{};
  l_path.resize(l_size);
  THROW_LAST_ERROR_IF(::GetModuleFileNameW(nullptr, l_path.data(), l_size) != l_size);
  auto l_cmd = fmt::format(LR"("{}" --service)", l_path);

  wil::unique_schandle l_unique_sc_handle_manager{THROW_IF_NULL_ALLOC(
      ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE | SC_MANAGER_LOCK)
  )};  // 打开服务控制管理器数据库，返回一个句柄
  // 创建一个服务
  wil::unique_schandle l_service_handle{THROW_IF_NULL_ALLOC(::CreateServiceW(
      l_unique_sc_handle_manager.get(), L"doodle_scan_win_service", L"doodle_scan_win_service", SERVICE_ALL_ACCESS,
      SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, l_cmd.c_str(), nullptr, nullptr, nullptr,
      nullptr, nullptr
  ))};

  // 添加服务说明
  SERVICE_DESCRIPTIONW l_service_description{};
  auto l_description = fmt::format(L"{} 扫瞄服务器资产并进行确认后提交数据库工具", L"doodle");
  l_service_description.lpDescription = l_description.data();
  THROW_IF_WIN32_BOOL_FALSE(
      ::ChangeServiceConfig2W(l_service_handle.get(), SERVICE_CONFIG_DESCRIPTION, &l_service_description)
  );
}

}  // namespace

void scan_win_service_t::start() {
  timer_  = std::make_shared<timer_t>(g_io_context());
  signal_ = std::make_shared<signal_t>(g_io_context(), SIGINT, SIGTERM);
  signal_->async_wait(boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), [](auto&&...) {
    g_io_context().stop();
  }));

  project_roots_ = {
      details::scan_category_t::project_root_t{"//192.168.10.250/public/DuBuXiaoYao_3", "独步逍遥"},
      {"//192.168.10.240/public/renjianzuideyi", "人间最得意"},
      {"//192.168.10.240/public/WuJinShenYu", "无尽神域"},
      {"//192.168.10.240/public/WuDiJianHun", "无敌剑魂"},
      {"//192.168.10.240/public/WanGuShenHua", "万古神话"},
      {"//192.168.10.240/public/LianQiShiWanNian", "炼气十万年"},
      {"//192.168.10.240/public/WGXD", "万古邪帝"},
      {"//192.168.10.240/public/LongMaiWuShen", "龙脉武神"},
      {"//192.168.10.218/WanYuFengShen", "万域封神"}
  };
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