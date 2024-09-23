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

const std::vector<project>& register_file_type::get_project_list() {
  winreg::RegKey l_key_root{};
  static std::vector<project> l_list{

      project{
          "独步逍遥", R"(//192.168.10.250/public/DuBuXiaoYao_3)", "DuBuXiaoYao", "DB", "V:/",
          R"(\\192.168.10.250\public\HouQi\1-DuBuXiaoYao\1_DBXY_TiJiaoWenJian\)"
      },

      project{
          "万古邪帝", R"(//192.168.10.240/public/WGXD)", "WanGuXieDi", "DW", "C:/sy/WGXD",
          R"(\\192.168.10.240\public\后期\WanGuXieDi\)"
      },

      project{
          "龙脉武神", R"(//192.168.10.240/public/LongMaiWuShen)", "LongMaiWuShen", "LM", R"(C:\sy\LongMaiWuShen)", ""
      },
      project{
          "炼气十万年", R"(//192.168.10.240/public/LianQiShiWanNian)", "LianQiShiWanNian", "LQ",
          R"(C:\sy\LianQiShiWanNian_8)", R"(\\192.168.10.240\public\后期\LianQiShiWanNian\)"
      },
      project{
          "人间最得意", R"(//192.168.10.240/public/renjianzuideyi)", "RenJianZuiDeYi", "RJ",
          R"(C:\sy\RenJianZuiDeYi_8)", ""
      },
      project{"无敌剑魂", R"(//192.168.10.240/public/WuDiJianHun)", "WuDiJianHun", "WD", R"(C:\sy\WuDiJianHun_8)", ""},
      project{
          "万古神话", R"(//192.168.10.240/public/WanGuShenHua)", "WanGuShenHua", "WG", "R:/",
          R"(\\192.168.10.240\public\后期\WanGuShenHua\)"
      },
      project{
          "无尽神域", R"(//192.168.10.240/public/WuJinShenYu)", "WuJinShenYu", "WJ", R"(C:\sy\WuJinShenYu_8)",
          R"(\\192.168.10.240\public\后期\WuJinShenYu)"
      },
      project{"万域封神", R"(//192.168.10.218/WanYuFengShen)", "WanYuFengShen", "WY", "U:/", ""},
      project{
          "双生武魂", R"(/192.168.10.240/public/SSWH)", "SSWH", "SS", R"(C:\sy\SSWH)", ""

      },
      project{
          "宗门里除了我都是卧底", R"(//192.168.10.240/public/ZMLCLWDSWD)", "ZMLCLWDSWD", "ZM", R"(C:\sy\ZMLCLWDSWD)",
          R"(\\192.168.10.240\public\后期\ZMLCLWDSWD\)"
      },
      project{
          "我的师兄太强了", R"(//192.168.10.240/public/WDSXTQL)", "WDSXTQL", "WD", R"(C:\sy\WDSXTQL)",
          R"(\\192.168.10.240\public\后期\WDSXTQL\)"
      }

  };
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
    l_path = boost::dll::program_location().parent_path();
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
    return R"(D:/doodle_snapshot/doodle_task.doodle_db)";
  }
}
}  // namespace doodle
