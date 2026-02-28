//
// Created by TD on 2024/2/29.
//

#include "app_service.h"

#include <doodle_core/lib_warp/boost_locale_warp.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <boost/asio.hpp>

#include <wil/resource.h>
#include <wil/result.h>
#include <windows.h>
namespace doodle {
app_service *app_service::g_this = nullptr;
void app_service::set_service_status(DWORD dw_current_state, DWORD dw_win32_exit_code, DWORD dw_wait_hint) {
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

DWORD WINAPI
app_service::service_ctrl_handler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) {
  switch (dwControl) {
    case SERVICE_CONTROL_STOP:
      boost::asio::post(g_io_context(), [lpContext]() { reinterpret_cast<app_service *>(lpContext)->stop_service(); });
      return NO_ERROR;
    case SERVICE_CONTROL_SHUTDOWN:
      boost::asio::post(g_io_context(), [lpContext]() {
        reinterpret_cast<app_service *>(lpContext)->shutdown_service();
      });
      return NO_ERROR;
    case SERVICE_CONTROL_INTERROGATE:
      return NO_ERROR;
    default:
      break;
  }
  return ERROR_CALL_NOT_IMPLEMENTED;
}
void WINAPI app_service::service_main(DWORD dwArgc, PWSTR *pszArgv) {
  app_service::g_this->h_service_status_ = THROW_LAST_ERROR_IF_NULL(
      ::RegisterServiceCtrlHandlerExW(L"doodle_win_service", service_ctrl_handler, app_service::g_this)
  );
  default_logger_raw()->log(log_loc(), level::warn, "注册服务成功");
  if (app_service::g_this->h_service_status_ == nullptr) {
    default_logger_raw()->log(log_loc(), level::err, "注册服务失败");
    return;
  }
  // 启动服务
  app_service::g_this->start_service();
  // 服务初始化
}

void app_service::install_service(
    const std::wstring &in_service_name, const std::wstring &in_display_name, const std::wstring &in_description,
    const std::wstring &in_command_line
) {
  FSys::path l_exe_path{};
  {
    WCHAR l_path[g_path_length]{};
    const auto l_ret = ::GetModuleFileNameW(nullptr, l_path, g_path_length);
    if (l_ret == 0) {
      THROW_LAST_ERROR();
    }
    l_exe_path = l_path;
  }
  auto l_cmd = fmt::format(
      LR"("{}" --{} {})", l_exe_path.generic_wstring(), conv::utf_to_utf<wchar_t>(g_service), in_command_line
  );
  default_logger_raw()->log(log_loc(), level::warn, "安装服务 {}", conv::utf_to_utf<char>(l_cmd));

  wil::unique_schandle l_unique_sc_handle_manager{THROW_LAST_ERROR_IF_NULL(
      ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE | SC_MANAGER_LOCK)
  )};  // 打开服务控制管理器数据库，返回一个句柄

  if (!user_name_.empty() && !user_name_.starts_with(L".\\")) {
    user_name_ = L".\\" + user_name_;
  }

  // 创建一个服务
  wil::unique_schandle l_service_handle{THROW_LAST_ERROR_IF_NULL(::CreateServiceW(
      l_unique_sc_handle_manager.get(), in_service_name.c_str(), in_display_name.c_str(), SERVICE_ALL_ACCESS,
      SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, l_cmd.c_str(), nullptr, nullptr, nullptr,
      user_name_.c_str(), password_.c_str()
  ))};

  // 添加服务说明
  SERVICE_DESCRIPTIONW l_service_description{};
  auto l_description                  = in_description;
  l_service_description.lpDescription = l_description.data();
  THROW_IF_WIN32_BOOL_FALSE(
      ::ChangeServiceConfig2W(l_service_handle.get(), SERVICE_CONFIG_DESCRIPTION, &l_service_description)
  );
  // 添加服务重启
  SERVICE_FAILURE_ACTIONS l_service_failure_actions{};
  l_service_failure_actions.dwResetPeriod = 60;
  THROW_IF_WIN32_BOOL_FALSE(
      ::ChangeServiceConfig2W(l_service_handle.get(), SERVICE_CONFIG_FAILURE_ACTIONS, &l_service_failure_actions)
  );
}

void app_service::uninstall_service(const std::wstring &in_service_name) {
  default_logger_raw()->log(log_loc(), level::warn, "卸载服务 doodle_scan_win_service");
  wil::unique_schandle l_unique_sc_handle_manager{THROW_LAST_ERROR_IF_NULL(
      ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE | SC_MANAGER_LOCK)
  )};  // 打开服务控制管理器数据库，返回一个句柄
  wil::unique_schandle l_service_handle{THROW_LAST_ERROR_IF_NULL(::OpenServiceW(
      l_unique_sc_handle_manager.get(), in_service_name.c_str(), SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE
  ))};

  // 查询服务状态
  SERVICE_STATUS l_service_status{};
  THROW_IF_WIN32_BOOL_FALSE(::QueryServiceStatus(l_service_handle.get(), &l_service_status));
  if (l_service_status.dwCurrentState == SERVICE_RUNNING || l_service_status.dwCurrentState == SERVICE_PAUSED) {
    // 尝试停止服务
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
  }
  THROW_IF_WIN32_BOOL_FALSE(::DeleteService(l_service_handle.get()));
}

std::int32_t app_service::run() {
  if (stop_) return 0;

  default_logger_raw()->log(
      log_loc(), level::info, "开始解析命令行 pos_args: {} flags: {} params: {}", arg_.pos_args(), arg_.flags(),
      arg_.params()
  );
  if (arg_[g_install]) {
    if (auto l_user = arg_(g_user); l_user) user_name_ = boost::locale::conv::utf_to_utf<wchar_t>(l_user.str());
    if (auto l_pass = arg_(g_password)) password_ = boost::locale::conv::utf_to_utf<wchar_t>(l_pass.str());
    try {
      install_service(service_name_, display_name_, description_, command_line_);
    }
    CATCH_LOG();
    return 0;
  }
  if (arg_[g_uninstall]) {
    try {
      uninstall_service(service_name_);
    }
    CATCH_LOG();
    return 0;
  }
  if (arg_[g_service]) {
    g_this = this;
    default_logger_raw()->log(log_loc(), level::warn, "启动服务");
    ::SERVICE_TABLE_ENTRY l_service_table_entry[]{
        {const_cast<LPWSTR>(L"doodle_scan_win_service"), reinterpret_cast<::LPSERVICE_MAIN_FUNCTIONW>(service_main)},
        {nullptr, nullptr}
    };
    THROW_IF_WIN32_BOOL_FALSE(::StartServiceCtrlDispatcherW(l_service_table_entry));
    default_logger_raw()->log(log_loc(), level::warn, "服务退出");
    return 0;
  }
  if (arg_[g_run]) {
    using signal_t = boost::asio::signal_set;

    auto signal_   = std::make_shared<signal_t>(g_io_context(), SIGINT, SIGTERM);
    signal_->async_wait(boost::asio::bind_cancellation_slot(
        app_base::Get().on_cancel.slot(),
        [](boost::system::error_code in_error_code, int in_sig) {
          default_logger_raw()->log(log_loc(), level::warn, "收到信号 {} {}", in_error_code.message(), in_sig);
          app_base::GetPtr()->stop_app();
        }
    ));
    facets_.emplace_back(signal_);

    app_base::run();
  }
  return 0;
}

void app_service::stop_app(std::int32_t in_exit_code) {
  app_base::stop_app(in_exit_code);
  //  if (thread_) thread_->join();
}

void app_service::start_service() {
  set_service_status(SERVICE_START_PENDING);
  thread_ = std::make_shared<std::thread>([this]() {
    if (stop_) return 0;
    try {
      g_io_context().run();
    } catch (...) {
      default_logger_raw()->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
    }
    try {
      g_io_context().run_for(std::chrono::milliseconds(10));
    } catch (...) {
      default_logger_raw()->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
    }
    return 0;
  });
  set_service_status(SERVICE_RUNNING);
}

void app_service::stop_service() {
  set_service_status(SERVICE_STOP_PENDING);
  boost::asio::post(g_io_context(), [this]() {
    stop_app();
    set_service_status(SERVICE_STOPPED);
  });
}
void app_service::shutdown_service() {
  set_service_status(SERVICE_STOP_PENDING);
  boost::asio::post(g_io_context(), [this]() {
    stop_app();
    set_service_status(SERVICE_STOPPED);
  });
}
void app_service::pause_service() {
  set_service_status(SERVICE_PAUSE_PENDING);
  if (LOG_LAST_ERROR_IF(::SuspendThread(thread_->native_handle()) == static_cast<DWORD>(-1))) {
    set_service_status(SERVICE_RUNNING);
  }
  set_service_status(SERVICE_PAUSED);
}
void app_service::resume_service() {
  set_service_status(SERVICE_CONTINUE_PENDING);
  if (LOG_LAST_ERROR_IF(::ResumeThread(thread_->native_handle()) == static_cast<DWORD>(-1))) {
    set_service_status(SERVICE_PAUSED);
  }
  set_service_status(SERVICE_RUNNING);
}
}  // namespace doodle