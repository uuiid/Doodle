//
// Created by TD on 2022/2/22.
//

#include "register_file_type.h"

#include <doodle_core/lib_warp/boost_locale_warp.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/project.h>

#include <winreg/WinReg.hpp>

namespace doodle {

register_file_type::register_file_type() = default;

void register_file_type::register_type() {
  winreg::RegKey l_key{};
  l_key.Create(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\doodle.main.1\\shell\\open\\command");
  l_key.SetStringValue({}, L"");
}
std::optional<database::ref_data> register_file_type::get_ref_uuid() {
  winreg::RegKey l_key{HKEY_LOCAL_MACHINE, L""};
  return std::optional<database::ref_data>();
}

FSys::path register_file_type::get_main_project() {
  try {
    winreg::RegKey l_key{};
    l_key.Open(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Doodle\MainConfig)", KEY_QUERY_VALUE | KEY_WOW64_64KEY);
    auto l_path = l_key.GetStringValue(LR"(main_project)");
    return l_path;

  } catch (const winreg::RegException& e) {
    default_logger_raw()->log(log_loc(), level::warn, "读取主工程路径失败 {}", e.what());
    return {};
  }
}

FSys::path register_file_type::get_update_path() {
  winreg::RegKey l_key{};
  l_key.Open(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Doodle\MainConfig)", KEY_QUERY_VALUE | KEY_WOW64_64KEY);
  auto l_path = l_key.GetStringValue(LR"(update_path)");
  return l_path;
}

std::vector<project> register_file_type::get_project_list() {
  winreg::RegKey l_key_root{};

  constexpr auto l_root     = LR"(SOFTWARE\Doodle\MainConfig\ProjectList)";
  constexpr auto l_root_fmt = LR"(SOFTWARE\Doodle\MainConfig\ProjectList\{})";
  l_key_root.Open(HKEY_LOCAL_MACHINE, l_root, KEY_QUERY_VALUE | KEY_WOW64_64KEY);

  std::vector<project> l_list{};

  for (auto&& l_sub : l_key_root.EnumSubKeys()) {
    winreg::RegKey l_key{};
    l_key.Open(HKEY_LOCAL_MACHINE, fmt::format(l_root_fmt, l_sub.c_str()), KEY_QUERY_VALUE | KEY_WOW64_64KEY);
    auto l_short  = conv::utf_to_utf<char>(l_sub);
    auto l_name   = conv::utf_to_utf<char>(l_key.GetStringValue({}));
    auto l_path   = conv::utf_to_utf<char>(l_key.GetStringValue(LR"(path)"));
    auto l_en_str = conv::utf_to_utf<char>(l_key.GetStringValue(LR"(en_str)"));
    l_list.emplace_back(std::move(l_short), std::move(l_name), std::move(l_path), std::move(l_en_str));
  }
  return l_list;
}

}  // namespace doodle
