/*
 * @Author: your name
 * @Date: 2020-11-16 19:05:15
 * @LastEditTime: 2020-11-30 10:53:01
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\main.cpp
 */
#include <DoodleConfig.h>
#include <DoodleLib/DoodleApp.h>
#include <Gui/main_windows.h>
#include <Gui/setting_windows.h>
#include <core/CoreSet.h>
#include <core/DoodleLib.h>
#include <fmt/ostream.h>
#include <libWarp/json_warp.h>
#include <rpc/RpcServerHandle.h>
#include <shellapi.h>

#include <boost/algorithm/string.hpp>
#include <exception>
#include <nana/fwd.hpp>
#include <nana/gui.hpp>
#include <nlohmann/json.hpp>
namespace doodle {

/**
 * @brief 这个是解析命令行专用的类
 * 
 */
class DOODLELIB_API command_line {
 public:
  int p_sql_port{3306};                     ///< mysql 端口
  int p_meta_rpc_port{60999};               ///< 元数据端口
  int p_file_rpc_port{60998};               ///< filesys 文件传输端口
  std::string p_sql_host{"192.168.20.60"};  ///< mysql数据库ip
  std::string p_sql_user{"deve"};           ///< mysql 用户名称
  std::string p_sql_password{"deve"};       ///< mysql 用户密码
  command_line()
      : p_sql_port(3306),
        p_meta_rpc_port(60999),
        p_file_rpc_port(60998),
        p_sql_host("192.168.20.60"),
        p_sql_user("deve"),
        p_sql_password("deve"),
        p_mk_link(),
        b_mklink(false),
        b_server(false){};

  /**
   * @brief 制作硬链接的属性
   * 
   */
  std::vector<std::pair<FSys::path, FSys::path> > p_mk_link;
  /**
   * @brief 使用时硬链接制作
   * 
   */
  bool b_mklink{};
  /**
   * @brief 是否时候启动服务器功能
   * 
   */
  bool b_server{};

  /**
   * @brief 获得简单的全局设置
   * 
   */
  inline void get_set() {
    if (!b_server)
      return;
    auto& set       = CoreSet::getSet();
    p_sql_port      = set.getSqlPort();
    p_meta_rpc_port = set.getMetaRpcPort();
    p_file_rpc_port = set.getFileRpcPort();
    p_sql_host      = set.getSqlHost();
    p_sql_user      = set.getSqlUser();
    p_sql_password  = set.getSqlPassword();
  };

  /**
   * @brief 将配置文件直接添加到全局设置中
   * 
   */

  inline void set_set() const {
    if (!b_server)
      return;

    auto& set = CoreSet::getSet();
    set.setSqlPort(p_sql_port);
    set.setMetaRpcPort(p_meta_rpc_port);
    set.setFileRpcPort(p_file_rpc_port);
    set.setSqlHost(p_sql_host);
    set.setSqlUser(p_sql_user);
    set.setSqlPassword(p_sql_password);
  };
  /**
   * @brief 转换为json
   * 
   * @param nlohmann_json_j 
   * @param nlohmann_json_t 
   */
  friend void to_json(nlohmann::json& nlohmann_json_j, const command_line& nlohmann_json_t) {
    nlohmann_json_j["b_server"]        = nlohmann_json_t.b_server;
    nlohmann_json_j["b_mklink"]        = nlohmann_json_t.b_mklink;
    nlohmann_json_j["p_sql_port"]      = nlohmann_json_t.p_sql_port;
    nlohmann_json_j["p_meta_rpc_port"] = nlohmann_json_t.p_meta_rpc_port;
    nlohmann_json_j["p_file_rpc_port"] = nlohmann_json_t.p_file_rpc_port;
    nlohmann_json_j["p_sql_host"]      = nlohmann_json_t.p_sql_host;
    nlohmann_json_j["p_sql_user"]      = nlohmann_json_t.p_sql_user;
    nlohmann_json_j["p_sql_password"]  = nlohmann_json_t.p_sql_password;
    nlohmann_json_j["p_mk_link"]       = nlohmann_json_t.p_mk_link;
  };
  /**
   * @brief 从json读取值
   * 
   * @param nlohmann_json_j 
   * @param nlohmann_json_t 
   */
  friend void from_json(const nlohmann::json& nlohmann_json_j, command_line& nlohmann_json_t) {
    try {
      nlohmann_json_j.at("b_server").get_to(nlohmann_json_t.b_server);
      nlohmann_json_j.at("b_mklink").get_to(nlohmann_json_t.b_mklink);
      if (nlohmann_json_t.b_server) {
        nlohmann_json_j.at("p_sql_port").get_to(nlohmann_json_t.p_sql_port);
        nlohmann_json_j.at("p_meta_rpc_port").get_to(nlohmann_json_t.p_meta_rpc_port);
        nlohmann_json_j.at("p_file_rpc_port").get_to(nlohmann_json_t.p_file_rpc_port);
        nlohmann_json_j.at("p_sql_host").get_to(nlohmann_json_t.p_sql_host);
        nlohmann_json_j.at("p_sql_user").get_to(nlohmann_json_t.p_sql_user);
        nlohmann_json_j.at("p_sql_password").get_to(nlohmann_json_t.p_sql_password);
      }
      if (nlohmann_json_t.b_mklink) {
        nlohmann_json_j.at("p_mk_link").get_to(nlohmann_json_t.p_mk_link);
      }
    } catch (const nlohmann::json::exception& error) {
      DOODLE_LOG_INFO(error.what());
      nlohmann_json_t.b_server = false;
    }
  };
};

doodle_app::doodle_app()
    : p_run_fun(),
      p_rpc_server_handle(),
      p_setting_windows() {
  init_opt();
}
void doodle_app::init() {
  int k_argc;
  auto k_argv = CommandLineToArgvW(GetCommandLine(), &k_argc);

  if (k_argc == 2) {
    auto k_path = FSys::path(k_argv[1]);
    if (!exists(k_path)) {
      DOODLE_LOG_WARN("没有找到配置文件: {}\n写入默认配置", k_path)
      FSys::ofstream k_file{k_path, std::ios::out};
      nlohmann::json k_json{};
      command_line config{};
      config.b_server = true;
      k_json.emplace_back(config);
      k_file << k_json;
      return;
    }
    FSys::ifstream k_file{k_path, std::ios::in};
    if (!k_file)
      return;
    auto p_info = nlohmann::json::parse(k_file)[0].get<command_line>();
    p_info.set_set();
    if (p_info.b_mklink) {
      p_run_fun = [p_info]() {
        for (auto& k_p : p_info.p_mk_link) {
          //          MklinkWidget::mklink(k_p.first, k_p.second);
        }
      };
    } else if (p_info.b_server) {
      run_server();
    }
  } else if (k_argc == 1) {
    p_run_fun = [this]() {
      this->run_gui();
    };
  }
  LocalFree(k_argv);
}
void doodle_app::run_server() {
  p_run_fun = [this]() {
    auto& set           = CoreSet::getSet();
    p_rpc_server_handle = std::make_shared<RpcServerHandle>();
    p_rpc_server_handle->runServer(set.getMetaRpcPort(), set.getFileRpcPort());
    nana::form _w{};
    _w.caption("关闭窗口停止服务器");
    _w.show();
    nana::exec();
  };
}
void doodle_app::run() {
  if (!p_run_fun)
    init();

  if (p_run_fun)
    p_run_fun();
}
void doodle_app::init_opt() {
}
void doodle_app::run_gui() {
  DoodleLib::Get().init_gui();

  main_windows k_main_windows{};
  k_main_windows.show();
  //  p_setting_windows = std::make_shared<setting_windows>(k_main_windows);

  nana::exec();
}

}  // namespace doodle
