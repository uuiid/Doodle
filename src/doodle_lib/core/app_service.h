//
// Created by TD on 2024/2/29.
//

#pragma once
#include <doodle_core/core/app_base.h>
namespace doodle {
class app_service : public app_base {
 public:
  static constexpr auto g_run{"run"};

 protected:
  static constexpr auto g_path_length{32767};
  static constexpr auto g_install{"install"};
  static constexpr auto g_uninstall{"uninstall"};
  static constexpr auto g_service{"service"};

  // 用户名
  static constexpr auto g_user{"user"};
  // 密码
  static constexpr auto g_password{"password"};

  void install_service(
      const std::wstring& in_service_name, const std::wstring& in_display_name, const std::wstring& in_description,
      const std::wstring& in_command_line
  );
  void uninstall_service(const std::wstring& in_service_name);

 public:
  std::wstring service_name_;
  std::wstring display_name_;
  std::wstring description_;
  std::wstring command_line_;

  // 用户名
  std::wstring user_name_;
  // 密码
  std::wstring password_;

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
  void stop_app(std::int32_t in_exit_code = 0) override;
};

/**
 * @brief 服务器基本的命令行类
 */
template <typename... Facet_>
class app_service_t : public app_service {
 public:
  app_service_t() : app_service() { run_facet(); };

  app_service_t(int argc, const char* const argv[]) : app_service(argc, argv) { run_facet(); }
  explicit app_service_t(std::int32_t argc, const wchar_t* const argv[]) : app_service(argc, argv) { run_facet(); }
  virtual ~app_service_t() override = default;

  void run_facet() {
    try {
      std::array<bool, sizeof...(Facet_)> l_r{
          Facet_{}(arg_, facets_)...,
      };
      stop_ = std::any_of(l_r.begin(), l_r.end(), [](bool i) { return i; });
    } catch (...) {
      default_logger_raw()->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
      default_logger_raw()->flush();
      stop_ = true;
    }
  }

 protected:
};

}  // namespace doodle