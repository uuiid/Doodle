//
// Created by TD on 2022/1/5.
//
#include <catch2/catch.hpp>
#include <doodle_lib/doodle_lib_all.h>
#include <Windows.h>
#include <doodle_lib/app/app.h>
#include <doodle_lib/platform/win/wnd_proc.h>

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

class test_get_font_data : public app {
 public:
};

TEST_CASE_METHOD(test_get_font_data, "test_get_font_data") {
  auto k_str = win::get_font();
  std::vector<FSys::path> l_r{};

  boost::copy(
      std::make_pair(FSys::directory_iterator{k_str}, FSys::directory_iterator{}) |
          boost::adaptors::filtered([](const FSys::path& in) {
            return in.extension() == ".ttf";
          }),
      std::back_inserter(l_r)
  );

  std::cout << fmt::to_string(fmt::join(l_r, "\n"));
}
