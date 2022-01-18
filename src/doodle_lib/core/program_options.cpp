//
// Created by TD on 2021/10/18.
//

#include "program_options.h"

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/doodle_app.h>
#include <doodle_lib/server/doodle_server.h>

namespace doodle {
program_options::program_options()
    : p_opt_all("doodle opt"),
      p_opt_file("doodle config file"),
      p_opt_gui("doodle config gui"),
      p_opt_general("doodle config general"),
      p_opt_advanced("doodle general config"),
      p_help(false),
      p_version(false),
      p_config_file(),
      p_max_thread(std::make_pair(false, std::thread::hardware_concurrency() - 2)),
      p_root(std::make_pair(false, "C:/")) {
  DOODLE_LOG_INFO("开始构建命令行");
  p_opt_general.add_options()(
      help_,
      boost::program_options::bool_switch(&p_help),
      "help")(
      version_,
      boost::program_options::bool_switch(&p_version),
      "显示版本")(
      config_file_,
      boost::program_options::value(&p_config_file),
      "配置文件的路径");

  p_opt_gui.add_options()(
      root_,
      boost::program_options::value(&p_root.second),
      "数据根目录");

  p_opt_advanced.add_options()(
      thread_max_,
      boost::program_options::value(&p_max_thread.second),
      "线程池大小\n(默认文硬件最大限制 - 1)");

  p_opt_all.add(p_opt_general).add(p_opt_gui).add(p_opt_advanced);
  p_opt_file.add(p_opt_gui).add(p_opt_advanced);
}
bool program_options::command_line_parser(const std::vector<string>& in_arg) {
  p_arg = in_arg;
  DOODLE_LOG_INFO("开始解析命令行");
  boost::program_options::command_line_parser k_p{in_arg};

  k_p.options(p_opt_all).allow_unregistered().style(
      boost::program_options::command_line_style::default_style |
      boost::program_options::command_line_style::allow_slash_for_short);

  auto k_opt = k_p.run();
  boost::program_options::store(k_opt, p_vm);

  if (p_vm.count(config_file)) {
    auto k_path = p_vm[config_file].as<FSys::path>();
    if (!k_path.empty() && FSys::exists(k_path)) {
      FSys::ifstream k_file{p_vm[config_file].as<FSys::path>()};
      if (k_file)
        boost::program_options::store(boost::program_options::parse_config_file(k_file, p_opt_file), p_vm);
    }
  }
  p_max_thread.first = p_vm.count(thread_max);
  p_root.first       = p_vm.count(root);
  boost::program_options::store(boost::program_options::parse_environment(p_opt_file, "doodle_"), p_vm);
  boost::program_options::notify(p_vm);
  using namespace std::literals;

  std::cout
      << (p_config_file.empty()
              ? "没有传入配置文件选项"s
              : fmt::format("使用配置 config_file : {}", p_config_file))
      << "\n"
      << (p_max_thread.first
              ? "没有传入线程池大小"s
              : fmt::format("使用配置 max_thread : {}", p_max_thread))
      << "\n"
      << (p_root.first ? "没有传入缓存根路径"s : fmt::format("使用配置 root : {}", p_root)) << "\n"
      << "开始初始化库基础(日志类和程序日期数据库)"
      << "\n"
      << std::endl;

  if (p_help) {
    std::cout << p_opt_all << std::endl;
  }
  if (p_version) {
    std::cout << fmt::format("doodle 版本是 {}.{}.{}.{} ",
                             Doodle_VERSION_MAJOR,
                             Doodle_VERSION_MINOR,
                             Doodle_VERSION_PATCH,
                             Doodle_VERSION_TWEAK)
              << std::endl;
  }

  if (!p_config_file.empty())
    DOODLE_LOG_INFO("配置文件解析为 config_file : {}", p_config_file);

  return true;
};

}  // namespace doodle
