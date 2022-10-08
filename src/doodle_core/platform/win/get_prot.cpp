//
// Created by TD on 2022/10/8.
//

#include "get_prot.h"
#include <Windows.h>
#include <winsock.h>
#include <iprtrmib.h>
#include <tlhelp32.h>
#include <iphlpapi.h>

#include <wil/result.h>

#include <doodle_core/logger/logger.h>
namespace doodle::win {

namespace {
struct MIB_TCPTABLE_free {
 public:
  void operator()(MIB_TCPTABLE2* in) {
    std::free(in);
  }
};
}  // namespace

std::uint32_t get_tcp_port(std::uint32_t id) {
  DOODLE_LOG_INFO("寻找线程id {}", id);
  DWORD l_err, dwSize;

  l_err = GetTcpTable2(nullptr, &dwSize, true);
  if (l_err != ERROR_INSUFFICIENT_BUFFER)
    THROW_WIN32(l_err);

  std::unique_ptr<MIB_TCPTABLE2, MIB_TCPTABLE_free> tcp_table{
      reinterpret_cast<MIB_TCPTABLE2*>(std::malloc(dwSize))};
  l_err = GetTcpTable2(tcp_table.get(), &dwSize, true);
  THROW_IF_WIN32_ERROR(l_err);

  std::uint32_t l_r_port{};
  for (auto i = 0;
       i < tcp_table->dwNumEntries;
       ++i) {
    if (auto& l_row = tcp_table->table[i];
        //        l_row.dwState == MIB_TCP_STATE_ESTAB &&
        l_row.dwOwningPid == id) {
      DOODLE_LOG_INFO("找到线程port dwLocalPort {}", l_row.dwLocalPort);
      DOODLE_LOG_INFO("找到线程port dwRemotePort {}", l_row.dwRemotePort);
      DOODLE_LOG_INFO("找到线程port dwRemoteAddr {}", l_row.dwRemoteAddr);
      DOODLE_LOG_INFO("找到线程port dwLocalAddr {}", l_row.dwLocalAddr);
      l_r_port = l_row.dwLocalPort;
      //      break;
    }
  }
  return l_r_port;
}

}  // namespace doodle::win
