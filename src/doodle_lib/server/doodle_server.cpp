//
// Created by TD on 2021/10/18.
//

#include "doodle_server.h"

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/external/service-base/ServiceInstaller.h>
#include <doodle_lib/rpc/rpc_server_handle.h>

namespace doodle {
std::wstring &doodle_server::server_name() {
  static std::wstring k_obj{L"doodle_rpc_server"};
  return k_obj;
}

doodle_server::doodle_server(PCWSTR pszServiceName,
                             BOOL fCanStop,
                             BOOL fCanShutdown,
                             BOOL fCanPauseContinue,
                             DWORD dwErrorEventId,
                             WORD wErrorCategoryId)
    : CServiceBase(pszServiceName,
                   fCanStop,
                   fCanShutdown,
                   fCanPauseContinue,
                   dwErrorEventId,
                   wErrorCategoryId) {
  p_h = new_object<rpc_server_handle>();
}

bool doodle_server::install_server() {
  InstallService(
      server_name().c_str(),
      L"doodle rpc",
      L"doodle rpc 服务器， 包括元数据服务器和文件服务器",
      L"--server",
      SERVICE_AUTO_START,
      nullptr,
      nullptr,
      nullptr,
      true);
}

bool doodle_server::uninstall_server() {
  UninstallService(server_name().c_str());
}

void doodle_server::OnStart(DWORD dwArgc, PWSTR *pszArgv) {
  auto &k_set = core_set::getSet();
  p_h->run_server(k_set.get_meta_rpc_port(), k_set.get_file_rpc_port());
}

void doodle_server::OnStop() {
  p_h->stop();
}
}  // namespace doodle