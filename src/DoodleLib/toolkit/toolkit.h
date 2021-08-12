/*
 * @Author: your name
 * @Date: 2020-11-16 15:39:08
 * @LastEditTime: 2020-12-07 10:19:50
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\toolkit.h
 */
#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {
/**
 * @brief 一些额外的没有什么关系的组件
 * 
 */
class toolkit {
 public:
  /**
   * @addtogroup doodle_windwos
   * @subsubsection install_ue_plugin 安装ue插件
   * 
   * @pre 安装ue插件需要在 @ref doodle_windwos_setting "设置" 中寻找到ue4的位置
   */
  /**
   * @addtogroup doodle_windwos
   * @subsubsection install_ue_project_plugin 安装ue项目插件
   * 
   * @b 安装ue项目插件是可以直接使用的，但是请注意版本问题
   */

  /**
   * @brief 安装ue插件
   * 
   * @param path 安装需要的目的地
   * 
   * @note 这里传入的必须是ue项目文件的父路径或者是ue的安装路径，
   * 是像 @ref doodle_windwos_setting "设置" 中定义的安装路径的 @b Engine 子文件夹
   */
  static void installUePath(const FSys::path &path);
  /**
   * @brief 安装maya插件,这个是安装到maya的doc文件中的
   * 
   */
  static void installMayaPath();
  /**
   * @brief 自动升级,
   * 
   * @return true 
   * @return false 
   */
  static bool update();
  /**
   * @brief 修改ue4缓存位置, 
   * 我们使用更改ue4配置文件,将 @b %ENGINEVERSIONAGNOSTICUSERDIR%DerivedDataCache
   * 替换为 @b %GAMEDIR%DerivedDataCache 的方法运行
   */
  static void modifyUeCachePath();
  /**
   * @brief 删除缓存
   * 我们适使用 @b SHGetKnownFolderPath 寻找到FOLDERID_LocalAppData文件夹后删除ue4目录中的缓存
   * 
   * @return true 
   * @return false 
   */
  static bool deleteUeCache();

 private:
  static FSys::path getUeInstallPath();
};

}  // namespace doodle
