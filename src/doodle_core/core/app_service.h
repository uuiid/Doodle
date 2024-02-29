//
// Created by TD on 2024/2/29.
//

#pragma once
#include <doodle_core/core/app_base.h>
namespace doodle {
class app_service : public app_base {
 protected:
  static constexpr auto g_path_length{32767};
  static constexpr auto g_install{"install"};
  static constexpr auto g_uninstall{"uninstall"};
  static constexpr auto g_service{"service"};
  static constexpr auto g_run{"run"};

  void install_service(
      const std::wstring& in_service_name, const std::wstring& in_display_name, const std::wstring& in_description,
      const std::wstring& in_command_line
  );
  void uninstall_service(const std::wstring& in_service_name);

  std::wstring service_name_;
  std::wstring display_name_;
  std::wstring description_;
  std::wstring command_line_;

 private:
  SERVICE_STATUS_HANDLE h_service_status_{};
  std::shared_ptr<std::thread> thread_;
  static DWORD WINAPI service_ctrl_handler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
  static void WINAPI service_main(DWORD dwArgc, PWSTR* pszArgv);
  static app_service* g_this;
  void set_service_status(DWORD dw_current_state, DWORD dw_win32_exit_code = NO_ERROR, DWORD dw_wait_hint = 0ul);
  void start_service();
  void stop_service();
  void shutdown_service();
  void pause_service();
  void resume_service();

 public:
  app_service() : app_base() {}
  app_service(int argc, const char* const argv[]) : app_base(argc, argv) {}
  explicit app_service(std::int32_t argc, const wchar_t* const argv[]) : app_base(argc, argv) {}
  virtual ~app_service() override = default;

  std::int32_t run() override;
  void stop_app(bool in_stop = true) override;
};
}  // namespace doodle