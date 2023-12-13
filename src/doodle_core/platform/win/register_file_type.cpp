//
// Created by TD on 2022/2/22.
//

#include "register_file_type.h"

#include <winreg/WinReg.hpp>

namespace doodle {

register_file_type::register_file_type() = default;

void register_file_type::register_type() {
  winreg::RegKey l_key{};
  l_key.Create(HKEY_CURRENT_USER, L"SOFTWARE\\Classes\\doodle.main.1\\shell\\open\\command");
  l_key.SetStringValue({}, L"");
}
std::optional<database::ref_data> register_file_type::get_ref_uuid() {
  winreg::RegKey l_key{HKEY_CURRENT_USER, L""};
  return std::optional<database::ref_data>();
}

FSys::path register_file_type::get_main_project() {
  winreg::RegKey l_key{};
  l_key.Open(HKEY_CURRENT_USER, LR"(SOFTWARE\Doodle\MainConfig)", KEY_QUERY_VALUE | KEY_WOW64_64KEY);
  auto l_path = l_key.GetStringValue(LR"(main_project)");
  return l_path;
}

FSys::path register_file_type::get_update_path() {
  winreg::RegKey l_key{};
  l_key.Open(HKEY_CURRENT_USER, LR"(SOFTWARE\Doodle\MainConfig)", KEY_QUERY_VALUE | KEY_WOW64_64KEY);
  auto l_path = l_key.GetStringValue(LR"(update_path)");
  return l_path;
}

}  // namespace doodle
