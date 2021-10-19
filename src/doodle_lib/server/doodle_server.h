//
// Created by TD on 2021/10/18.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/external/service-base/ServiceBase.h>

namespace doodle {

class DOODLELIB_API doodle_server {
  rpc_server_handle_ptr p_h;

 public:
  // Register the executable for a service with the Service Control Manager
  // (SCM). After you call Run(ServiceBase), the SCM issues a Start command,
  // which results in a call to the OnStart method in the service. This
  // method blocks until the service has stopped.
  static BOOL Run(doodle_server &service);

  // A command line to be passed to the service when it runs
  void SetCommandLine(int argc, PWSTR argv[]) {
    m_argc = argc;
    m_argv = argv;
  }

  // Stop the service.
  void Stop();

 protected:
  // When implemented in a derived class, executes when a Start command is
  // sent to the service by the SCM or when the operating system starts
  // (for a service that starts automatically). Specifies actions to take
  // when the service starts.
  virtual void OnStart(DWORD dwArgc, PWSTR *pszArgv);

  // When implemented in a derived class, executes when a Stop command is
  // sent to the service by the SCM. Specifies actions to take when a
  // service stops running.
  virtual void OnStop();

  // When implemented in a derived class, executes when a Pause command is
  // sent to the service by the SCM. Specifies actions to take when a
  // service pauses.
  virtual void OnPause();

  // When implemented in a derived class, OnContinue runs when a Continue
  // command is sent to the service by the SCM. Specifies actions to take
  // when a service resumes normal functioning after being paused.
  virtual void OnContinue();

  // When implemented in a derived class, executes when the system is
  // shutting down. Specifies what should occur immediately prior to the
  // system shutting down.
  virtual void OnShutdown();

  // Set the service status and report the status to the SCM.
  void SetServiceStatus(DWORD dwCurrentState,
                        DWORD dwWin32ExitCode = NO_ERROR,
                        DWORD dwWaitHint      = 0);

  // Log a message to the Application event log.
  virtual void WriteLogEntry(PCWSTR pszMessage, WORD wType, DWORD dwEventId = 0, WORD wCategory = 0);

  // Entry point for the service. It registers the handler function for the
  // service and starts the service.
  static void WINAPI ServiceMain(DWORD dwArgc, LPWSTR *lpszArgv);

  // The function is called by the SCM whenever a control code is sent to
  // the service.
  static void WINAPI ServiceCtrlHandler(DWORD dwCtrl);

  // Start the service.
  void Start(DWORD dwArgc, PWSTR *pszArgv);

  // Pause the service.
  void Pause();

  // Resume the service after being paused.
  void Continue();

  // Execute when the system is shutting down.
  void Shutdown();

  // The singleton service instance.
  static doodle_server *s_service;

  // The name of the service
  PCWSTR m_name;

  // Command line for the service
  int m_argc;
  PWSTR *m_argv;

  // The status of the service
  SERVICE_STATUS m_status;

  // The service status handle
  SERVICE_STATUS_HANDLE m_statusHandle;

  // This is used for error logging into Windows Event Log
  DWORD m_dwErrorEventId;
  WORD m_wErrorCategoryId;

 public:
  doodle_server(PCWSTR pszServiceName,
                BOOL fCanStop          = TRUE,
                BOOL fCanShutdown      = TRUE,
                BOOL fCanPauseContinue = FALSE,
                DWORD dwErrorEventId   = 0,
                WORD wErrorCategoryId  = 0);
  ~doodle_server() = default;

  // static bool install_server();
  // static bool uninstall_server();

  //  protected:
  //   void OnStart(DWORD dwArgc, PWSTR* pszArgv) ;
  //   void OnStop() ;
};

}  // namespace doodle