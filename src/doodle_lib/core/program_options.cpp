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
      p_opt_server("doodle config server"),
      p_opt_general("doodle config general"),
      p_opt_advanced("doodle general config"),
      p_lib(new_object<doodle_lib>()),
      p_use_gui(true),
      p_server(false),
      p_install(false),
      p_uninstall(false),
      p_help(false),
      p_version(false),
      p_config_file(core_set::getSet().get_cache_root() / "doodle.ini"),
      p_max_thread(core_set::getSet().p_max_thread),
      p_root(core_set::getSet().get_root()),
      p_mysql_ip(core_set::getSet().get_sql_host()),
      p_mysql_user(core_set::getSet().get_sql_user()),
      p_mysql_pow(core_set::getSet().get_sql_password()),
      p_rpc_setver_ip(core_set::getSet().get_server_host()),
      p_mysql_port(core_set::getSet().get_sql_port()),
      p_rpc_file_port(core_set::getSet().get_file_rpc_port()),
      p_rpc_meta_port(core_set::getSet().get_meta_rpc_port()) {
  DOODLE_LOG_INFO("开始初始化基本配置");

  core_set_init k_init{};
  k_init.find_cache_dir();
  p_root = core_set::getSet().get_root();

  DOODLE_LOG_INFO("开始构建命令行");
  p_opt_general.add_options()(
      "help,h",
      boost::program_options::bool_switch(&p_help)->default_value(p_help),
      "help")(
      "version,v",
      boost::program_options::bool_switch(&p_version)->default_value(p_version),
      "显示版本")(
      "config_file",
      boost::program_options::value(&p_config_file),
      "配置文件的路径");

  p_opt_gui.add_options()(
      "gui,g",
      boost::program_options::bool_switch(&p_use_gui)->default_value(p_use_gui),
      "运行gui")(
      "root",
      boost::program_options::value(&p_root)->default_value(p_root),
      "数据根目录")(
      "rpc_address",
      boost::program_options::value(&p_rpc_setver_ip)->default_value(p_rpc_setver_ip),
      "rpc地址");

  p_opt_server.add_options()(
      "install,i",
      boost::program_options::bool_switch(&p_install)->default_value(p_install),
      "安装服务")(
      "uninstall",
      boost::program_options::bool_switch(&p_uninstall)->default_value(p_uninstall),
      "卸载服务")(
      "server,s",
      boost::program_options::bool_switch(&p_server)->default_value(p_server),
      "启动服务")(
      "mysql_address",
      boost::program_options::value(&p_mysql_ip)->default_value(p_mysql_ip),
      "mysql数据库地址")(
      "mysql_user",
      boost::program_options::value(&p_mysql_user)->default_value(p_mysql_user),
      "mysql数据库用户名")(
      "mysql_password",
      boost::program_options::value(&p_mysql_pow)->default_value(p_mysql_pow),
      "mysql数据库密码");

  p_opt_advanced.add_options()(
      "rpc_file_prot",
      boost::program_options::value(&p_rpc_file_port)->default_value(p_rpc_file_port),
      "rpc文件服务器端口")(
      "rpc_meta_prot",
      boost::program_options::value(&p_rpc_meta_port)->default_value(p_rpc_meta_port),
      "rpc元数据服务器端口")(
      "mysql_prot",
      boost::program_options::value(&p_mysql_port)->default_value(p_mysql_port),
      "mysql数据库端口")(
      "thread_max",
      boost::program_options::value(&p_max_thread)->default_value(p_max_thread),
      "线程池大小\n(默认文硬件最大限制 - 2)");

  p_opt_all.add(p_opt_general).add(p_opt_gui).add(p_opt_server).add(p_opt_advanced);
  p_opt_file.add(p_opt_gui).add(p_opt_server).add(p_opt_advanced);
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

  if (p_vm.count("config_file")) {
    auto k_path = p_vm["config_file"].as<FSys::path>();
    if (!k_path.empty() && FSys::exists(k_path)) {
      FSys::ifstream k_file{p_vm["config_file"].as<FSys::path>()};
      if (k_file)
        boost::program_options::store(boost::program_options::parse_config_file(k_file, p_opt_file), p_vm);
    }
  }

  boost::program_options::store(boost::program_options::parse_environment(p_opt_file, "doodle_"), p_vm);
  boost::program_options::notify(p_vm);

  std::cout
      << fmt::format("使用配置 config_file : {}", p_config_file) << "\n"
      << fmt::format("使用配置 运行gui : {}", p_use_gui) << "\n"
      << fmt::format("使用配置 运行服务 : {}", p_server) << "\n"
      << fmt::format("使用配置 安装服务 : {}", p_install) << "\n"
      << fmt::format("使用配置 max_thread : {}", p_max_thread) << "\n"
      << fmt::format("使用配置 root : {}", p_root) << "\n"
      << fmt::format("使用配置 mysql_ip : {}", p_mysql_ip) << "\n"
      << fmt::format("使用配置 mysql_user : {}", p_mysql_user) << "\n"
      << fmt::format("使用配置 mysql_pow : {}", p_mysql_pow) << "\n"
      << fmt::format("使用配置 rpc_setver_ip : {}", p_rpc_setver_ip) << "\n"
      << fmt::format("使用配置 mysql_port : {}", p_mysql_port) << "\n"
      << fmt::format("使用配置 rpc_file_port : {}", p_rpc_file_port) << "\n"
      << fmt::format("使用配置 rpc_meta_port : {}", p_rpc_meta_port) << "\n"
      << "开始初始化库基础(日志类和程序日期数据库)"
      << "\n"
      << std::endl;

  if (p_help) {
    std::cout << p_opt_all << std::endl;
    p_use_gui = false;
  }
  if (p_version) {
    std::cout << fmt::format("doodle 版本是 {}.{}.{}.{} ",
                             Doodle_VERSION_MAJOR,
                             Doodle_VERSION_MINOR,
                             Doodle_VERSION_PATCH,
                             Doodle_VERSION_TWEAK)
              << std::endl;
    p_use_gui = false;
  }

  auto& set = core_set::getSet();
  core_set_init k_init{};
  k_init.read_file();

  set.p_max_thread = p_max_thread;
  set.set_root(p_root);
  set.set_sql_host(p_mysql_ip);
  set.set_sql_user(p_mysql_user);
  set.set_sql_password(p_mysql_pow);
  set.set_sql_port(p_mysql_port);
  set.set_server_host(p_rpc_setver_ip);
  set.set_file_rpc_port(p_rpc_file_port);
  set.set_meta_rpc_port(p_rpc_meta_port);

  DOODLE_LOG_INFO("配置文件解析为 config_file : {}", p_config_file);
  DOODLE_LOG_INFO("使用配置 运行gui : {}", p_use_gui)
  DOODLE_LOG_INFO("使用配置 运行服务 : {}", p_server)
  DOODLE_LOG_INFO("使用配置 安装服务 : {}", p_install)
  DOODLE_LOG_INFO("使用配置 max_thread : {}", p_max_thread);
  DOODLE_LOG_INFO("使用配置 root : {}", p_root);
  DOODLE_LOG_INFO("使用配置 mysql_ip : {}", p_mysql_ip);
  DOODLE_LOG_INFO("使用配置 mysql_user : {}", p_mysql_user);
  DOODLE_LOG_INFO("使用配置 mysql_pow : {}", p_mysql_pow);
  DOODLE_LOG_INFO("使用配置 rpc_setver_ip : {}", p_rpc_setver_ip);
  DOODLE_LOG_INFO("使用配置 mysql_port : {}", p_mysql_port);
  DOODLE_LOG_INFO("使用配置 rpc_file_port : {}", p_rpc_file_port);
  DOODLE_LOG_INFO("使用配置 rpc_meta_port : {}", p_rpc_meta_port);
  DOODLE_LOG_INFO("初始化完成");

  return true;
}
doodle_app_ptr program_options::make_app() {
  if (p_install) {
    doodle_server::install_server();
    return nullptr;
  }
  if (p_uninstall) {
    doodle_server::uninstall_server();
    return nullptr;
  }
  if (p_server) {
    DOODLE_LOG_INFO("初始化服务器日志");
    logger_ctrl::get_log().set_log_name("doodle_server.txt");
    DOODLE_LOG_INFO("开始运行服务");
    doodle_server k_ser{L"doodle_rpc_server"};
    k_ser.SetCommandLine(0, nullptr);
    if (!doodle_server::Run(k_ser)) {
      DWORD dwErr = GetLastError();
      DOODLE_LOG_ERROR("Service failed to run with error code: {}", dwErr);
    }
    return nullptr;
  }
  if (p_use_gui) {
    DOODLE_LOG_INFO("初始化gui日志");
    logger_ctrl::get_log().set_log_name("doodle_gui.txt");
    DOODLE_LOG_INFO("开始gui初始化");
    core_set_init k_init{};
    k_init.write_file();
    p_lib->init_gui();
    DOODLE_LOG_INFO("开始gui显示gui界面");
    auto k_gui = new_object<doodle_app>();
    return k_gui;
  }

  return nullptr;
};

}  // namespace doodle
