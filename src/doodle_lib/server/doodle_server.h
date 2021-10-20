//
// Created by TD on 2021/10/18.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/external/service-base/ServiceBase.h>

namespace doodle {

class DOODLELIB_API doodle_server : public CServiceBase {
  rpc_server_handle_ptr p_h;

  static std::wstring& server_name();

 public:
  doodle_server(PCWSTR pszServiceName,
                BOOL fCanStop          = TRUE,
                BOOL fCanShutdown      = TRUE,
                BOOL fCanPauseContinue = FALSE,
                DWORD dwErrorEventId   = 0,
                WORD wErrorCategoryId  = 0);
  ~doodle_server() = default;

  static bool install_server();
  static bool uninstall_server();

 protected:
  void OnStart(DWORD dwArgc, PWSTR* pszArgv) override;
  void OnStop() override;
};

}  // namespace doodle