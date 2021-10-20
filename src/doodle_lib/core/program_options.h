//
// Created by TD on 2021/10/18.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/program_options.hpp>
namespace doodle {

class DOODLELIB_API program_options {
 private:
  FSys::path p_config_file;
  std::int32_t p_max_thread;
  FSys::path p_root;
  string p_mysql_ip;
  string p_mysql_user;
  string p_mysql_pow;
  std::int32_t p_mysql_port;
  string p_rpc_setver_ip;
  std::int32_t p_rpc_file_port;
  std::int32_t p_rpc_meta_port;

  bool p_use_gui;
  bool p_server;
  bool p_install;
  bool p_uninstall;
  bool p_help;
  bool p_version;

  doodle_lib_ptr p_lib;
  
  std::vector<string> p_arg;
 private:
  /**
   * @brief 所有选项， 命令行选项
   *
   */
  boost::program_options::options_description p_opt_all;
  /**
   * @brief 解析配置文件时的选项
   *
   */
  boost::program_options::options_description p_opt_file;

  /**
   * @brief gui选项
   *
   */
  boost::program_options::options_description p_opt_gui;
  /**
   * @brief 服务器选项
   *
   */
  boost::program_options::options_description p_opt_server;
  /**
   * @brief 一般选项
   *
   */
  boost::program_options::options_description p_opt_general;
  /**
   * @brief 高级设置
   *
   */
  boost::program_options::options_description p_opt_advanced;

  boost::program_options::variables_map p_vm;

 public:
  program_options();

  /**
   * @brief 解析命令行
   *
   * @param argc 传入的命令行参数
   * @param argv 传入的命令行参数
   * @return true 解析成功
   * @return false 解析失败
   */
  inline bool command_line_parser(int argc, const char* argv[]) {
    string_list k_str{argv, argv + argc};
    return command_line_parser(k_str);
  };
  bool command_line_parser(const std::vector<string>& in_arg);
  inline bool command_line_parser(const LPSTR& in_arg) {
    auto k_str = boost::program_options::split_winmain(in_arg);
    return command_line_parser(k_str);
  };

  doodle_app_ptr make_app();
};

}  // namespace doodle
