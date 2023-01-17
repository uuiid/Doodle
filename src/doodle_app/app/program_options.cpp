//
// Created by TD on 2021/10/18.
//

#include "program_options.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/logger/logger.h>

#include <doodle_app/lib_warp/boost_fmt_progrm_opt.h>

#include <boost/locale.hpp>
namespace doodle::win {
std::vector<std::string> get_command_line() {
  auto l_args =
      boost::program_options::split_winmain(std::string{boost::locale::conv::utf_to_utf<char>(GetCommandLineW())});
  l_args.erase(l_args.cbegin());
  return l_args;
}
}  // namespace doodle::win

namespace doodle::details {
program_options::program_options()
    : p_opt_all("doodle opt"),
      p_opt_general("doodle config general"),
      p_help(false),
      p_version(false),
      p_config_file(),
      p_ue4outpath(),
      p_ue4Project(),
      p_arg(win::get_command_line()) {
  DOODLE_LOG_INFO("开始构建命令行");

  p_opt_positional.add(input_project, 1);

  p_opt_general.add_options()(help_, boost::program_options::bool_switch(&p_help), "help")(
      version_, boost::program_options::bool_switch(&p_version), "显示版本"
  )(input_project, boost::program_options::value(&p_project_path),
    "初始打开的项目文件")(config_file, boost::program_options::value(&p_config_file), "配置文件的路径")(
      ue4outpath, boost::program_options::value(&p_ue4outpath), "导出ue4导入配置文件的路径"
  )(ue4Project, boost::program_options::value(&p_ue4Project), "ue4项目路径");
}
bool program_options::command_line_parser() {
  p_opt_all.add(p_opt_general);

  DOODLE_LOG_INFO("开始解析命令行 [{}]", fmt::join(p_arg, "  "));
  boost::program_options::command_line_parser k_p{p_arg};

  k_p.positional(p_opt_positional)
      .options(p_opt_all)
      .allow_unregistered()
      .style(
          boost::program_options::command_line_style::default_style |
          boost::program_options::command_line_style::allow_slash_for_short
      );

  auto k_opt = k_p.run();
  boost::program_options::store(k_opt, p_vm);

  if (p_vm.count(config_file)) {
    auto k_path = p_vm[config_file].as<FSys::path>();
    if (!k_path.empty() && FSys::exists(k_path)) {
      FSys::ifstream k_file{p_vm[config_file].as<FSys::path>()};
      if (k_file) boost::program_options::store(boost::program_options::parse_config_file(k_file, p_opt_all), p_vm);
    }
  }

  boost::program_options::store(boost::program_options::parse_environment(p_opt_all, "doodle_"), p_vm);
  boost::program_options::notify(p_vm);

  if (p_help) {
    DOODLE_LOG_WARN("{}", p_opt_all);
  }
  if (p_version) DOODLE_LOG_WARN("版本 {}", version::build_info::get().version_str);

  return true;
}
void program_options::build_opt(const std::string& in_name_face) {
  p_opt_all.add_options(
  )(in_name_face.c_str(), boost::program_options::bool_switch(&facet_model[in_name_face]),
    fmt::format("启动 {} 模式", in_name_face).c_str());
}
void program_options::add_opt(const boost::program_options::options_description& in_opt) { p_opt_all.add(in_opt); }
bool program_options::operator[](const std::string& in_key) const { return p_vm.count(in_key); };

}  // namespace doodle::details
