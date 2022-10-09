//
// Created by TD on 2021/10/18.
//

#include "program_options.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/logger/logger.h>
#include <doodle_app/lib_warp/boost_fmt_progrm_opt.h>

namespace boost::filesystem {
template <class CharT>
void validate(
    boost::any& v,
    const std::vector<std::basic_string<CharT>>& values,
    ::boost::filesystem::path*,
    std::int32_t
) {
  using namespace boost::program_options;

  // Make sure no previous assignment to 'a' was made.
  validators::check_first_occurrence(v);
  // Extract the first string from 'values'. If there is more than
  // one string, it's an error, and exception will be thrown.
  const std::basic_string<CharT>& s = validators::get_single_string(values);

  // Do regex match and convert the interesting part to
  // int.
  if (s[0] == '"') {
    v = boost::any{::doodle::FSys::path{s.substr(1, s.size() - 2)}};
  } else {
    v = boost::any{
        ::doodle::FSys::path{s}};
  }
}

}  // namespace boost::filesystem

namespace doodle {
program_options::program_options()
    : p_opt_all("doodle opt"),
      p_opt_general("doodle config general"),
      p_help(false),
      p_version(false),
      p_config_file(),
      p_ue4outpath(),
      p_ue4Project() {
  DOODLE_LOG_INFO("开始构建命令行");

  p_opt_positional.add(input_project, 1);

  p_opt_general.add_options()(
      help_,
      boost::program_options::bool_switch(&p_help),
      "help"
  )(
      version_,
      boost::program_options::bool_switch(&p_version),
      "显示版本"
  )(
      input_project,
      boost::program_options::value(&p_project_path),
      "初始打开的项目文件"
  )(
      config_file,
      boost::program_options::value(&p_config_file),
      "配置文件的路径"
  )(
      ue4outpath,
      boost::program_options::value(&p_ue4outpath),
      "导出ue4导入配置文件的路径"
  )(
      ue4Project,
      boost::program_options::value(&p_ue4Project),
      "ue4项目路径"
  );
}
bool program_options::command_line_parser(const std::vector<std::string>& in_arg) {
  p_opt_all.add(p_opt_general);

  p_arg = in_arg;
  DOODLE_LOG_INFO("开始解析命令行 [{}]", fmt::join(in_arg, "  "));
  boost::program_options::command_line_parser k_p{in_arg};

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
      if (k_file)
        boost::program_options::store(boost::program_options::parse_config_file(k_file, p_opt_all), p_vm);
    }
  }

  boost::program_options::store(boost::program_options::parse_environment(p_opt_all, "doodle_"), p_vm);
  boost::program_options::notify(p_vm);

  if (p_help) {
    DOODLE_LOG_WARN("{}", p_opt_all);
  }
  if (p_version)
    DOODLE_LOG_WARN("版本 {}", version::build_info::get().version_str);

  return true;
}
void program_options::build_opt(const std::string& in_name_face) {
  p_opt_all.add_options()(
      in_name_face.c_str(),
      boost::program_options::bool_switch(&facet_model[in_name_face]),
      fmt::format("启动 {} 模式", in_name_face).c_str()
  );
}
void program_options::add_opt(const boost::program_options::options_description& in_opt) {
  p_opt_all.add(in_opt);
}
bool program_options::operator[](const std::string& in_key) const {
  return p_vm.count(in_key);
};

}  // namespace doodle
