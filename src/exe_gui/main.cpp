/*
 * @Author: your name
 * @Date: 2020-11-16 19:05:15
 * @LastEditTime: 2020-11-30 10:53:01
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\main.cpp
 */
#include <doodle_lib/facet/create_move_facet.h>
#include <doodle_lib/facet/main_facet.h>
#include <doodle_lib/facet/rpc_server_facet.h>
// #include <doodle_lib/DoodleApp.h>
// #include <boost/locale.hpp>
/**
 * @mainpage doodle 工具包
 * @tableofcontents
 *
 * @defgroup 主要程序 主要程序
 * @tableofcontents
 * @brief 主要的程序逻辑
 * @defgroup 主要菜单栏 主要菜单栏
 * @brief 所有的功能入口
 * @defgroup 主要状态栏 主要状态栏
 * @brief 软件中的各种关键状态的显示
 * @defgroup 项目窗口 项目窗口
 * @brief 单个软件窗口中使用的项目
 * @defgroup 过滤窗口 过滤窗口
 * @brief 查找各种实体类型
 * @defgroup 实体窗口 实体窗口
 * @brief 显示实体的基本信息
 * @defgroup 编辑窗口 编辑窗口
 * @brief 编辑实体的各种组件
 * @defgroup 设置窗口 设置窗口
 * @brief 设置软件的各种运行参数
 * @defgroup 项目设置窗口 项目设置窗口
 * @brief 项目中的各种配置选项, 每个项目不同
 */

// extern "C" int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR strCmdLine, int nCmdShow) try {
extern "C" int main() try {
  using main_app = doodle::app_command<doodle::main_facet, doodle::facet::create_move_facet>;
  main_app app{};
  try {
    return app.run();
  } catch (const std::exception& err) {
    DOODLE_LOG_WARN(boost::diagnostic_information(boost::diagnostic_information(err)));
    return 1;
  }
} catch (...) {
  return 1;
}
