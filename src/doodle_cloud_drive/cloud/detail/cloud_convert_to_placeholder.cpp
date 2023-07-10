//
// Created by td_main on 2023/7/10.
//

#include "cloud_convert_to_placeholder.h"

#include "boost/asio/post.hpp"
// clang-format off
#include <windows.h>
#include <wil/result.h>
#include <sddl.h>
#include <unknwn.h>
#include <winternl.h>
#include <comutil.h>
#include <oleidl.h>
#include <ntstatus.h>
#include <cfapi.h>
// clang-format on

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>

#include <magic_enum.hpp>
namespace doodle::detail {
using unique_cf_hfile = wil::unique_any_handle_invalid<decltype(&::CfCloseHandle), ::CfCloseHandle>;

void cloud_convert_to_placeholder::async_run() {
  boost::asio::post(executor_, [self = shared_from_this()]() { self->async_convert_to_placeholder(); });
}
void cloud_convert_to_placeholder::async_convert_to_placeholder() {
  WIN32_FIND_DATA l_find_Data;

  wil::unique_hfind l_hfind{
      ::FindFirstFileExW(child_path_.c_str(), FindExInfoBasic, &l_find_Data, FindExSearchNameMatch, nullptr, 0)};

  if (!l_hfind.is_valid()) {
    ntstatus_ = NTSTATUS_FROM_WIN32(::GetLastError());
    return;
  }
  CF_PLACEHOLDER_STATE l_state = ::CfGetPlaceholderStateFromFindData(&l_find_Data);
  switch (l_state) {
    case CF_PLACEHOLDER_STATE_NO_STATES:
    case CF_PLACEHOLDER_STATE_PLACEHOLDER:
    case CF_PLACEHOLDER_STATE_SYNC_ROOT:
    case CF_PLACEHOLDER_STATE_ESSENTIAL_PROP_PRESENT:
    case CF_PLACEHOLDER_STATE_IN_SYNC:
    case CF_PLACEHOLDER_STATE_PARTIAL:
    case CF_PLACEHOLDER_STATE_PARTIALLY_ON_DISK: {
      unique_cf_hfile l_file_h{};

      LOG_IF_FAILED(::CfOpenFileWithOplock(child_path_.c_str(), CF_OPEN_FILE_FLAG_EXCLUSIVE, l_file_h.put()));

      if (l_file_h.is_valid()) {
        return;
        LOG_LAST_ERROR();
      }
      CF_CONVERT_FLAGS l_flags =
          (l_state == CF_PLACEHOLDER_STATE_IN_SYNC ? CF_CONVERT_FLAG_MARK_IN_SYNC : CF_CONVERT_FLAG_NONE);

      if (FSys::is_directory(child_path_)) {
        l_flags |= CF_CONVERT_FLAG_ENABLE_ON_DEMAND_POPULATION;
      }

      LOG_IF_FAILED(::CfConvertToPlaceholder(
          l_file_h.get(), server_path_.c_str(), server_path_.native().size() * sizeof(wchar_t), l_flags, nullptr,
          nullptr
      ));
      break;
    }
    default:
      DOODLE_LOG_INFO("Unknown state: {} {}", child_path_, magic_enum::enum_name(l_state));
      break;
  }
}

}  // namespace doodle::detail