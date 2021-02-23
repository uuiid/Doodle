#pragma once

#include "corelib/core_global.h"
#include <map>
#include <boost/filesystem.hpp>

DOODLE_NAMESPACE_S

enum class Department {
  None_,
  Executive,
  Light,
  VFX,
  modle,
  rig,
  Anm,
  direct,
  paint
};

struct synPath_struct {
  dpath local;
  dpath server;
};

/*
 *全局静态设置类
 */

class CORE_API coreSet {
 public:
  static coreSet &getSet();

  DOODLE_DISABLE_COPY(coreSet)
  //初始化函数
  void init();
  void reInit();
  void initdb();

  void appendEnvironment() const;

  //获得运行程序目录
  static dpath program_location();
  static dpath program_location(const dpath &path);

  //同步目录时的本地路径
  [[nodiscard]] const dpath getSynPathLocale() const;
  void setSynPathLocale(const dpath &syn_path);
  void setSynPathLocale(const QString &syn_path);

  // user设置
  [[nodiscard]] dstring getUser() const;
  [[nodiscard]] dstring getUser_en() const;
  void setUser(const dstring &value);

  //部门设置
  [[nodiscard]] dstring getDepartment() const;
  void setDepartment(const dstring &value);

  //同步集数设置
  [[nodiscard]] int getSyneps() const;
  void setSyneps(int value);

  //项目名称设置
  dstring getProjectname();
  [[nodiscard]] std::pair<int, std::string> projectName() const;
  [[nodiscard]] dstringList getAllPrjName() const;
  void setProjectname(const std::string &value);

  //缓存路径
  [[nodiscard]] dpath getCacheRoot() const;

  // doc路径
  [[nodiscard]] dpath getDoc() const;

  void writeDoodleLocalSet();

  static dstring toIpPath(const dstring &path);

 private:
  //私有化构造函数
  coreSet();
  //获得缓存磁盘路径
  void getCacheDiskPath();
  //获得服务器上的统一设置
  void getServerSetting();
  //获得本地的有限设置
  void getSetting();
  //转换为ip路径

 private:
  const static dstring settingFileName;

  //用户名称
  dstring user;
  //部门
  Department department;

  //项目名称
  std::string project;
  
  fileSys::path synPath;

  fileSys::path cacheRoot;
  fileSys::path doc;

};

DOODLE_NAMESPACE_E
