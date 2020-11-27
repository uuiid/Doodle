/*
 * @Author: your name
 * @Date: 2020-11-16 15:39:08
 * @LastEditTime: 2020-11-27 11:34:02
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\toolkit.h
 */
#pragma once
#include <doodle_global.h>
#include <core_global.h>

DOODLE_NAMESPACE_S
class toolkit {
 public:
  static void openPath(const doCore::fileSqlInfoPtr &info_ptr,
                       const bool &openEx);
  static void installUePath(const std::string &path);
  static bool update();

 private:
};

DOODLE_NAMESPACE_E