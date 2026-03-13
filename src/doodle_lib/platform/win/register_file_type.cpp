//
// Created by TD on 2022/2/22.
//

#include "register_file_type.h"

#include <doodle_core/metadata/project.h>

#include <doodle_lib/lib_warp/boost_locale_warp.h>
#include <doodle_lib/logger/logger.h>

#include <boost/dll.hpp>

#include <winreg/WinReg.hpp>


namespace doodle {

register_file_type::register_file_type() = default;

FSys::path register_file_type::program_location() {
#ifdef NDEBUG
  return boost::dll::program_location().parent_path();
#else
  FSys::path l_path;
  try {
    winreg::RegKey l_key{};
    l_key.Open(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Doodle\MainConfig)", KEY_QUERY_VALUE | KEY_WOW64_64KEY);
    l_path = l_key.GetStringValue(LR"(install_dir)");
  } catch (const winreg::RegException& e) {
    default_logger_raw()->log(log_loc(), level::warn, "读取程序路径失败 {}, 返回dll路径", e.what());
    l_path = boost::dll::program_location().parent_path();
  }

  return l_path;
#endif
}
}  // namespace doodle
