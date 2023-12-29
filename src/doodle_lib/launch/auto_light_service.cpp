//
// Created by TD on 2023/12/29.
//

#include "auto_light_service.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <wil/resource.h>
#include <wil/result.h>
#include <windows.h>
namespace doodle::launch {

namespace {
static constexpr auto g_install{"install"};
static constexpr auto g_uninstall{"uninstall"};
static constexpr auto g_service{"service"};

void install_scan_win_service() {
  DWORD l_size{};
  l_size = ::GetModuleFileNameW(nullptr, nullptr, l_size);
  THROW_LAST_ERROR_IF(::GetLastError() != ERROR_INSUFFICIENT_BUFFER);

  std::wstring l_path{};
  l_path.resize(l_size);
  THROW_LAST_ERROR_IF(::GetModuleFileNameW(nullptr, l_path.data(), l_size) != l_size);
  auto l_cmd = fmt::format(LR"("{}" --{})", l_path, conv::utf_to_utf<wchar_t>(g_service));

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

class auto_light_service_impl_t {
 public:
  static auto_light_service_impl_t *g_this;
  auto_light_service_impl_t() { g_this = this; }
  ~auto_light_service_impl_t() = default;

  static auto_light_service_impl_t &GetThis() { return *g_this; }

  SERVICE_STATUS_HANDLE h_service_status_{};
  std::shared_ptr<std::thread> thread_;

  void set_service_status(DWORD dw_current_state, DWORD dw_win32_exit_code = NO_ERROR, DWORD dw_wait_hint = 0ul) {
    static DWORD dw_check_point = 1;
    ::SERVICE_STATUS l_service_status{};
    l_service_status.dwServiceType   = SERVICE_WIN32_OWN_PROCESS;
    l_service_status.dwCurrentState  = dw_current_state;
    l_service_status.dwWin32ExitCode = dw_win32_exit_code;
    l_service_status.dwWaitHint      = dw_wait_hint;
    l_service_status.dwControlsAccepted =
        (((dw_current_state == SERVICE_START_PENDING) || (dw_current_state == SERVICE_STOP_PENDING))
             ? 0
             : SERVICE_ACCEPT_STOP) |
        SERVICE_ACCEPT_SHUTDOWN;
    l_service_status.dwCheckPoint =
        ((dw_current_state == SERVICE_RUNNING) || (dw_current_state == SERVICE_STOPPED)) ? 0 : dw_check_point++;
    THROW_IF_WIN32_BOOL_FALSE(::SetServiceStatus(h_service_status_, &l_service_status));
  }

  void start() {
    set_service_status(SERVICE_START_PENDING);
    thread_ = std::make_shared<std::thread>([]() { app_base::Get().run(); });
    set_service_status(SERVICE_RUNNING);
  }
  void stop() {
    set_service_status(SERVICE_STOP_PENDING);
    app_base::Get().stop_app();
    set_service_status(SERVICE_STOPPED);
  }
  void shutdown() {
    set_service_status(SERVICE_STOP_PENDING);
    app_base::Get().stop_app();
    set_service_status(SERVICE_STOPPED);
  }
};
DWORD WINAPI service_ctrl_handler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) {
  switch (dwControl) {
    case SERVICE_CONTROL_STOP:
      boost::asio::post(g_io_context(), [lpContext]() {
        reinterpret_cast<auto_light_service_impl_t *>(lpContext)->stop();
      });
      return NO_ERROR;
    case SERVICE_CONTROL_SHUTDOWN:
      boost::asio::post(g_io_context(), [lpContext]() {
        reinterpret_cast<auto_light_service_impl_t *>(lpContext)->shutdown();
      });
      return NO_ERROR;
    case SERVICE_CONTROL_INTERROGATE:
      return NO_ERROR;
    default:
      break;
  }
  return ERROR_CALL_NOT_IMPLEMENTED;
}
void WINAPI service_main(DWORD dwArgc, PWSTR *pszArgv) {
  auto_light_service_impl_t::GetThis().h_service_status_ = THROW_IF_NULL_ALLOC(::RegisterServiceCtrlHandlerExW(
      L"doodle_scan_win_service", service_ctrl_handler, auto_light_service_impl_t::g_this
  ));
  // 启动服务
  auto_light_service_impl_t::GetThis().start();
  // 服务初始化
}
}  // namespace
bool auto_light_service_t::operator()(const argh::parser &in_arh, std::vector<std::shared_ptr<void>> &in_vector) {
  if (in_arh[g_install]) {
    install_scan_win_service();
    return true;
  }
  if (in_arh[g_uninstall]) {
    uninstall_scan_win_service();
    return true;
  }
  if (in_arh[g_service]) {
    auto l_auto_light_service_impl_ptr = std::make_shared<auto_light_service_impl_t>();
    in_vector.emplace_back(l_auto_light_service_impl_ptr);

    ::SERVICE_TABLE_ENTRY l_service_table_entry[]{
        {L"doodle_scan_win_service", reinterpret_cast<::LPSERVICE_MAIN_FUNCTIONW>(service_main)}, {nullptr, nullptr}
    };
    THROW_IF_WIN32_BOOL_FALSE(::StartServiceCtrlDispatcherW(l_service_table_entry));
    //    auto scan_win_service_ptr_ = std::make_shared<scan_win_service_t>();
    //    g_ctx().get<core_sig>().project_end_open.connect([scan_win_service_ptr_]() { scan_win_service_ptr_->start();
    //    }); g_ctx().get<database_n::file_translator_ptr>()->async_open(register_file_type::get_main_project());
    //    in_vector.emplace_back(scan_win_service_ptr_);
  }
  return true;
}

}  // namespace doodle::launch