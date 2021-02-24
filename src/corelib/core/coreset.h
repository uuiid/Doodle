#pragma once

#include <corelib/core_global.h>
#include <corelib/core/Project.h>
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
  fileSys::path local;
  fileSys::path server;
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

  void appendEnvironment() const;

  //获得运行程序目录
  static fileSys::path program_location();
  static fileSys::path program_location(const fileSys::path &path);

  //同步目录时的本地路径
  [[nodiscard]] const fileSys::path getSynPathLocale() const;
  void setSynPathLocale(const fileSys::path &syn_path);

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
  std::shared_ptr<Project> getProject();
  std::vector<std::shared_ptr<Project>> getAllProjects();
  void setProject(const std::shared_ptr<Project> &projectRoot);

  //缓存路径
  [[nodiscard]] fileSys::path getCacheRoot() const;

  // doc路径
  [[nodiscard]] fileSys::path getDoc() const;

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

 private:
  const static dstring settingFileName;

  //项目名称
  std::map<fileSys::path, std::shared_ptr<Project>> p_projects;
  std::shared_ptr<Project> p_currentProject;
  //用户名称
  dstring user;
  //部门
  Department department;

  fileSys::path synPath;

  fileSys::path cacheRoot;
  fileSys::path doc;
};

DOODLE_NAMESPACE_E
