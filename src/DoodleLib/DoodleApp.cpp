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
#include <DoodleLib/Metadata/MetadataWidget.h>
#include <DoodleLib/Server/ServerWidget.h>
#include <DoodleLib/SettingWidght/settingWidget.h>
#include <DoodleLib/mainWidght/MklinkWidget.h>
#include <DoodleLib/mainWidght/tool_windows.h>
#include <Gui/main_windows.h>
#include <Gui/setting_windows.h>
#include <fmt/ostream.h>
#include <libWarp/json_warp.h>
#include <rpc/RpcServerHandle.h>
#include <shellapi.h>
#include <wx/cmdline.h>
#include <wx/wxprec.h>

#include <boost/algorithm/string.hpp>
#include <exception>
#include <nana/fwd.hpp>
#include <nana/gui.hpp>
#include <nlohmann/json.hpp>
namespace doodle {

class DOODLELIB_API command_line {
 public:
  int p_sql_port{};            ///< mysql 端口
  int p_meta_rpc_port{};       ///< 元数据端口
  int p_file_rpc_port{};       ///< filesys 文件传输端口
  std::string p_sql_host;      ///< mysql数据库ip
  std::string p_sql_user;      ///< mysql 用户名称
  std::string p_sql_password;  ///< mysql 用户密码

  std::vector<std::pair<FSys::path, FSys::path> > p_mk_link;

  bool b_mklink{};
  bool b_server{};

  inline void get_set() {
    auto& set       = CoreSet::getSet();
    p_sql_port      = set.getSqlPort();
    p_meta_rpc_port = set.getMetaRpcPort();
    p_file_rpc_port = set.getFileRpcPort();
    p_sql_host      = set.getSqlHost();
    p_sql_user      = set.getSqlUser();
    p_sql_password  = set.getSqlPassword();
  };

  inline void set_set() const {
    auto& set = CoreSet::getSet();
    set.setSqlPort(p_sql_port);
    set.setMetaRpcPort(p_meta_rpc_port);
    set.setFileRpcPort(p_file_rpc_port);
    set.setSqlHost(p_sql_host);
    set.setSqlUser(p_sql_user);
    set.setSqlPassword(p_sql_password);
  };

  friend void to_json(nlohmann::json& nlohmann_json_j, const command_line& nlohmann_json_t) {
    if (nlohmann_json_t.b_server) {
      nlohmann_json_j["p_sql_port"]      = nlohmann_json_t.p_sql_port;
      nlohmann_json_j["p_meta_rpc_port"] = nlohmann_json_t.p_meta_rpc_port;
      nlohmann_json_j["p_file_rpc_port"] = nlohmann_json_t.p_file_rpc_port;
      nlohmann_json_j["p_sql_host"]      = nlohmann_json_t.p_sql_host;
      nlohmann_json_j["p_sql_user"]      = nlohmann_json_t.p_sql_user;
      nlohmann_json_j["p_sql_password"]  = nlohmann_json_t.p_sql_password;
    } else if (nlohmann_json_t.b_mklink) {
      nlohmann_json_j["p_mk_link"] = nlohmann_json_t.p_mk_link;
    }
  };
  friend void from_json(const nlohmann::json& nlohmann_json_j, command_line& nlohmann_json_t) {
    try {
      nlohmann_json_j.at("p_sql_port").get_to(nlohmann_json_t.p_sql_port);
      nlohmann_json_j.at("p_meta_rpc_port").get_to(nlohmann_json_t.p_meta_rpc_port);
      nlohmann_json_j.at("p_file_rpc_port").get_to(nlohmann_json_t.p_file_rpc_port);
      nlohmann_json_j.at("p_sql_host").get_to(nlohmann_json_t.p_sql_host);
      nlohmann_json_j.at("p_sql_user").get_to(nlohmann_json_t.p_sql_user);
      nlohmann_json_j.at("p_sql_password").get_to(nlohmann_json_t.p_sql_password);
    } catch (const nlohmann::json::exception& error) {
      DOODLE_LOG_INFO(error.what());
      nlohmann_json_t.b_server = false;
    }
    nlohmann_json_t.b_server = true;
    try {
      nlohmann_json_j.at("p_mk_link").get_to(nlohmann_json_t.p_mk_link);
    } catch (const nlohmann::json::exception& error) {
      DOODLE_LOG_INFO(error.what());
      nlohmann_json_t.b_mklink = false;
    }
    nlohmann_json_t.b_mklink = true;
  };
};

doodle_app::doodle_app()
    : p_run_fun(),
      p_rpc_server_handle(std::make_shared<RpcServerHandle>()),
      p_setting_windows() {
  init_opt();
}
void doodle_app::init() {
  int k_argc;
  auto k_argv = CommandLineToArgvW(GetCommandLine(), &k_argc);

  if (k_argc == 2) {
    auto k_path = FSys::path(k_argv[1]);
    FSys::ifstream k_file{k_path, std::ios::in};
    if (!k_file)
      return;
    auto p_info = nlohmann::json::parse(k_file).get<command_line>();
    if (p_info.b_mklink) {
      p_run_fun = [p_info]() {
        for (auto& k_p : p_info.p_mk_link) {
          MklinkWidget::mklink(k_p.first, k_p.second);
        }
      };
    } else if (p_info.b_server) {
      p_run_fun = [this, p_info]() {
        p_info.set_set();
        auto& set = CoreSet::getSet();
        p_rpc_server_handle->runServer(p_info.p_meta_rpc_port, p_info.p_file_rpc_port);
      };
    }
  } else if (k_argc == 1) {
    p_run_fun = [this]() {
      this->gui_run();
    };
  }
  LocalFree(k_argv);
}
void doodle_app::run() {
  init();
  if (p_run_fun)
    p_run_fun();
}
void doodle_app::init_opt() {
}
void doodle_app::gui_run() {
  CoreSet::getSet().guiInit();

  main_windows k_main_windows{};
  k_main_windows.show();
//  p_setting_windows = std::make_shared<setting_windows>(k_main_windows);

  nana::exec();
}

}  // namespace doodle
