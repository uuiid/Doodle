//
// Created by TD on 2021/10/18.
//

#pragma once

#include <doodle_app/doodle_app_fwd.h>

#include <boost/program_options.hpp>
// #include <Windows.h>

namespace doodle::win {
std::vector<std::string> get_command_line();
}

namespace doodle::details {
class DOODLE_APP_API program_options : boost::noncopyable {
 public:
  FSys::path p_config_file;
  std::string p_project_path;
  std::string p_ue4outpath;
  std::string p_ue4Project;

  bool p_help;
  bool p_version;

  std::vector<std::string> p_arg;
  std::map<std::string, bool> facet_model;

 private:
  static constexpr char input_project[] = "input_project";
  static constexpr char help_[]         = "help,h";
  static constexpr char version[]       = "version";
  static constexpr char version_[]      = "version,v";
  static constexpr char config_file[]   = "config_file";
  static constexpr char ue4outpath[]    = "ue4outpath";
  static constexpr char ue4Project[]    = "ue4Project";

  /// @brief 所有选项， 命令行选项
  boost::program_options::options_description p_opt_all;
  /// @brief 一般选项
  boost::program_options::options_description p_opt_general;
  /// @brief 位置选项
  boost::program_options::positional_options_description p_opt_positional;

  boost::program_options::variables_map p_vm;

 public:
  program_options();

  void build_opt(const std::string& in_name_face);

  void add_opt(const boost::program_options::options_description& in_opt);
  /**
   * @brief 解析命令行
   *
   * @return true 解析成功
   * @return false 解析失败
   */
  bool command_line_parser();
  /// @brief 测试是否存在值
  bool operator[](const std::string& in_key) const;
};
}  // namespace doodle::details