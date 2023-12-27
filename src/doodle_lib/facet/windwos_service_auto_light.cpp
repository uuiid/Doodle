//
// Created by TD on 2023/12/26.
//

#include "windwos_service_auto_light.h"

#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/platform/win/register_file_type.h>

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

void uninstall_scan_win_service() {
  wil::unique_schandle l_unique_sc_handle_manager{THROW_IF_NULL_ALLOC(
      ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE | SC_MANAGER_LOCK)
  )};  // 打开服务控制管理器数据库，返回一个句柄
  wil::unique_schandle l_service_handle{THROW_IF_NULL_ALLOC(::OpenServiceW(
      l_unique_sc_handle_manager.get(), L"doodle_scan_win_service", SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE
  ))};

  // 尝试停止服务
  SERVICE_STATUS l_service_status{};
  THROW_IF_WIN32_BOOL_FALSE(::ControlService(l_service_handle.get(), SERVICE_CONTROL_STOP, &l_service_status));
  std::this_thread::sleep_for(chrono::seconds{1});
  while (::QueryServiceStatus(l_service_handle.get(), &l_service_status)) {
    if (l_service_status.dwCurrentState == SERVICE_STOP_PENDING) {
      std::this_thread::sleep_for(chrono::seconds{1});
    } else {
      break;
    }
  }
  if (l_service_status.dwCurrentState != SERVICE_STOPPED) {
    default_logger_raw()->log(log_loc(), level::err, "停止服务失败");
    THROW_LAST_ERROR();
  }
  THROW_IF_WIN32_BOOL_FALSE(::DeleteService(l_service_handle.get()));
}
}  // namespace

void windows_service_auto_light_facet_t::add_program_options() {}

bool windows_service_auto_light_facet_t::post() {
  scan_win_service_ptr_ = std::make_shared<scan_win_service_t>();
  g_ctx().get<core_sig>().project_end_open.connect([this]() { scan_win_service_ptr_->start(); });
  g_ctx().get<database_n::file_translator_ptr>()->async_open(register_file_type::get_main_project());
  return true;
}

void windows_service_auto_light_facet_t::install_server() { install_scan_win_service(); }

}  // namespace doodle