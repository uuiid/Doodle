//
// Created by TD on 2022/2/22.
//

#include "register_file_type.h"
#include <lib_warp/WinReg.hpp>

namespace doodle {

register_file_type::register_file_type() {
}
void register_file_type::register_type() {
  winreg::RegKey l_key{};
  l_key.Create(HKEY_CURRENT_USER,L"SOFTWARE\\Classes\\doodle.main.1\\shell\\open\\command");
  l_key.SetStringValue({},L"");
}

}  // namespace doodle