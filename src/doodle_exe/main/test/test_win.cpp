//
// Created by TD on 2022/1/5.
//
#include <catch2/catch.hpp>
#include <doodle_lib/doodle_lib_all.h>
#include <Windows.h>

using namespace doodle;

TEST_CASE("win get dir", "[win]") {
  auto k_buff_size = GetLogicalDriveStringsW(0, nullptr);
  std::cout << "get buff size : " << k_buff_size << std::endl;
  std::unique_ptr<wchar_t[]> p_buff{new wchar_t[k_buff_size]};
  GetLogicalDriveStringsW(k_buff_size, p_buff.get());

  std::cout << "wcahr_t size : " << sizeof(wchar_t) << std::endl;

  for (auto l_i = p_buff.get(); *l_i != 0; l_i += (sizeof(wchar_t) * 2)) {
    auto driveType = GetDriveTypeW(l_i);
    switch (driveType) {
      case DRIVE_REMOVABLE:
      case DRIVE_FIXED:
      case DRIVE_REMOTE: {
        FSys::path l_p{l_i};
        std::cout << l_p.generic_string() << "  -> is ok"
                  << "\n";
        break;
      }
      case DRIVE_UNKNOWN:
      case DRIVE_NO_ROOT_DIR:
      case DRIVE_CDROM:
      case DRIVE_RAMDISK:
      default:
        break;
    }
  }
}

TEST_CASE("win get dir2 ", "[win]") {
  std::cout << fmt::to_string(fmt::join(win::list_drive(), "\n")) << std::endl;
}
