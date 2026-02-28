//
// Created by TD on 2022/1/5.
//

#include "list_drive.h"

#include "wil/result_macros.h"
#include <Windows.h>
#include <wil/result.h>
namespace doodle::win {

std::vector<FSys::path> list_drive() {
  std::vector<FSys::path> l_r{};
  auto k_buff_size = GetLogicalDriveStringsW(0, nullptr);
  std::unique_ptr<wchar_t[]> p_buff{new wchar_t[k_buff_size]};

  THROW_LAST_ERROR_IF(ERROR_SUCCESS == GetLogicalDriveStringsW(k_buff_size, p_buff.get()));

  for (auto l_i = p_buff.get(); *l_i != 0; l_i += (sizeof(wchar_t) * 2)) {
    auto driveType = GetDriveTypeW(l_i);
    switch (driveType) {
      case DRIVE_REMOVABLE:
      case DRIVE_FIXED:
      case DRIVE_REMOTE:
        l_r.emplace_back(l_i);
        break;
      case DRIVE_UNKNOWN:
      case DRIVE_NO_ROOT_DIR:
      case DRIVE_CDROM:
      case DRIVE_RAMDISK:
      default:
        break;
    }
  }
  return l_r;
}
}  // namespace doodle::win
