/*
 * @Author: your name
 * @Date: 2020-11-16 15:39:08
 * @LastEditTime: 2020-12-07 10:19:50
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\toolkit.h
 */
#pragma once
#include <doodle_GUI/doodle_global.h>

DOODLE_NAMESPACE_S
class toolkit {
 public:
  // static void openPath(const fileSqlInfoPtr &info_ptr,
  //                      const bool &openEx);
  static void openPath(const FSys::path &path);
  static void installUePath(const FSys::path &path);
  static void installMayaPath();
  static bool update();

  static void modifyUeCachePath();
  static bool deleteUeCache();

 private:
  static FSys::path getUeInstallPath();
};

DOODLE_NAMESPACE_E