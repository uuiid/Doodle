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
class DOODLELIB_API doodle_app {
  std::function<void ()> p_run_fun;
  RpcServerHandlePtr p_rpc_server_handle;
  std::shared_ptr<setting_windows> p_setting_windows;
  void init_opt();

 public:
  doodle_app();
  void run();
  void init();
  void run_gui();
  void run_server();
};




}  // namespace doodle
