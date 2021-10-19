//
// Created by TD on 2021/10/18.
//

#include "doodle_server.h"

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/rpc/rpc_server_handle.h>
namespace doodle {
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

void doodle_server::OnStart(DWORD dwArgc, PWSTR* pszArgv) {
  auto& k_set = core_set::getSet();
  p_h->run_server(k_set.get_meta_rpc_port(), k_set.get_file_rpc_port());
}

void doodle_server::OnStop() {
  p_h->stop();
}

}  // namespace doodle