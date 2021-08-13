//
// Created by TD on 2021/5/7.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {
class SettingWidght;
class systemTray;
class tool_windows;
class MetadataWidget;
class ServerWidget;
class setting_windows;
/**
 * @brief 主要的运行类, 包括gui和服务器启动
 *
 */
class DOODLELIB_API doodle_app : public details::no_copy {
  std::function<void()> p_run_fun;
  RpcServerHandlePtr p_rpc_server_handle;
  std::shared_ptr<setting_windows> p_setting_windows;
  void init_opt();

  void add_signal_fun();

 public:
  doodle_app();
  ~doodle_app();
  /**
   * @brief 根据传入的命令行选择运行
   *
   */
  void run();
  /**
   * @brief 初始化
   *
   */
  void init();
  /**
   * @brief 运行gui
   *
   */
  void run_gui();
  /**
   * @brief 运行 rpc 和 元数据 服务器
   *
   */
  void run_server();
};

}  // namespace doodle
