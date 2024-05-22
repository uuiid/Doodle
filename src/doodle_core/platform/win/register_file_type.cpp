//
// Created by TD on 2022/2/22.
//

#include "register_file_type.h"

#include <doodle_core/lib_warp/boost_locale_warp.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/project.h>

#include <boost/dll.hpp>

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
    return {R"(//192.168.10.218/Doodletemp/db_file/doodle_main.doodle_db)"};
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
  std::vector<project> l_list{};
  try {
    l_key_root.Open(HKEY_LOCAL_MACHINE, l_root, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_64KEY);
    for (auto&& l_sub : l_key_root.EnumSubKeys()) {
      winreg::RegKey l_key{};
      l_key.Open(HKEY_LOCAL_MACHINE, fmt::format(l_root_fmt, l_sub.c_str()), KEY_QUERY_VALUE | KEY_WOW64_64KEY);
      auto l_short  = conv::utf_to_utf<char>(l_sub);
      auto l_name   = conv::utf_to_utf<char>(l_key.GetStringValue({}));
      auto l_path   = conv::utf_to_utf<char>(l_key.GetStringValue(LR"(path)"));
      auto l_en_str = conv::utf_to_utf<char>(l_key.GetStringValue(LR"(en_str)"));
      auto l_local  = conv::utf_to_utf<char>(l_key.GetStringValue(LR"(local)"));
      l_list.emplace_back(std::move(l_name), std::move(l_path), std::move(l_en_str), std::move(l_short), l_local);
    }
  } catch (const winreg::RegException& e) {
    l_list.emplace_back("独步逍遥", R"(//192.168.10.250/public/DuBuXiaoYao_3)", "DuBuXiaoYao", "DB", "V:/");
    l_list.emplace_back("万古邪帝", R"(//192.168.10.240/public/WGXD)", "WanGuXieDi", "DW", "C:/sy/WGXD");
    l_list.emplace_back(
        "龙脉武神", R"(//192.168.10.240/public/LongMaiWuShen)", "LongMaiWuShen", "LM", R"(C:\sy\LongMaiWuShen)"
    );
    l_list.emplace_back(
        "炼气十万年", R"(//192.168.10.240/public/LianQiShiWanNian)", "LianQiShiWanNian", "LQ",
        R"(C:\sy\LianQiShiWanNian_8)"
    );
    l_list.emplace_back(
        "人间最得意", R"(//192.168.10.240/public/renjianzuideyi)", "RenJianZuiDeYi", "RJ", R"(C:\sy\RenJianZuiDeYi_8)"
    );
    l_list.emplace_back(
        "无敌剑魂", R"(//192.168.10.240/public/WuDiJianHun)", "WuDiJianHun", "WD", R"(C:\sy\WuDiJianHun_8)"
    );
    l_list.emplace_back("万古神话", R"(//192.168.10.240/public/WanGuShenHua)", "WanGuShenHua", "WG", "R:/");
    l_list.emplace_back(
        "无尽神域", R"(//192.168.10.240/public/WuJinShenYu)", "WuJinShenYu", "WJ", R"(C:\sy\WuJinShenYu_8)"
    );
    l_list.emplace_back("万域封神", R"(//192.168.10.218/WanYuFengShen)", "WanYuFengShen", "WY", "U:/");
    l_list.emplace_back("双生武魂", R"(/192.168.10.240/public/SSWH)", "SSWH", "SS", R"(C:\sy\SSWH)");
  }
  return l_list;
}

FSys::path register_file_type::program_location() {
  FSys::path l_path;
  try {
    winreg::RegKey l_key{};
    l_key.Open(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Doodle\MainConfig)", KEY_QUERY_VALUE | KEY_WOW64_64KEY);
    l_path = l_key.GetStringValue(LR"(install_dir)");
  } catch (const winreg::RegException& e) {
    default_logger_raw()->log(log_loc(), level::warn, "读取程序路径失败 {}, 返回dll路径", e.what());
    l_path = boost::dll::program_location();
    // std::wstring l_path_str{MAX_PATH, L'\0'};
    // auto l_r = GetModuleFileNameW(nullptr, l_path_str.data(), MAX_PATH);
    // if (l_r) {
    //   l_path = FSys::path{conv::utf_to_utf<char>(l_path_str)};
    //   l_path = l_path.parent_path();
    // } else {
    //   auto l_ec = GetLastError();
    //   for (auto i = 2; i < 1025 && l_ec == ERROR_INSUFFICIENT_BUFFER; i *= 2) {
    //     l_path_str.resize(MAX_PATH * i);
    //     l_r = GetModuleFileNameW(nullptr, l_path_str.data(), MAX_PATH * i);
    //     if (l_r) {
    //       l_path_str.resize(l_r);
    //       l_path = FSys::path{conv::utf_to_utf<char>(l_path_str)};
    //       l_path = l_path.parent_path();
    //       break;
    //     }
    //     l_ec = GetLastError();
    //   }
    // }
  }
  return l_path;
}
std::string register_file_type::get_server_address() {
  try {
    winreg::RegKey l_key{};
    l_key.Open(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Doodle\MainConfig)", KEY_QUERY_VALUE | KEY_WOW64_64KEY);
    auto l_path = l_key.GetStringValue(LR"(server_address)");
    return boost::locale::conv::utf_to_utf<char>(l_path);
  } catch (const winreg::RegException& e) {
    default_logger_raw()->log(log_loc(), level::warn, "读取服务器地址失败 {}", e.what());
    return "192.168.40.181";
  }
}

FSys::path register_file_type::get_server_snapshot_path() {
  try {
    winreg::RegKey l_key{};
    l_key.Open(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Doodle\MainConfig)", KEY_QUERY_VALUE | KEY_WOW64_64KEY);
    auto l_path = l_key.GetStringValue(LR"(server_snapshot)");
    return l_path;
  } catch (const winreg::RegException& e) {
    default_logger_raw()->log(log_loc(), level::warn, "读取服务器快照路径失败 {}", e.what());
    return R"(D:/doodle_snapshot)";
  }
}
}  // namespace doodle
