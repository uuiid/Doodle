#pragma once

#include "core_global.h"
#include <map>
#include <boost/filesystem.hpp>
CORE_NAMESPACE_S

enum class dep {
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

class CORE_API coreSet{

 public:
  static coreSet &getSet();

  coreSet &operator=(const coreSet &s) = delete;
  coreSet(const coreSet &s) = delete;

  //初始化函数
  void init();
  void initdb();
  //获得同步路径
  synPathListPtr getSynDir();
  //同步目录时的本地路径
  [[nodiscard]] const dpath getSynPathLocale() const;
  void setSynPathLocale(const dpath &syn_path);
  void setSynPathLocale(const QString &syn_path);
  //MySQL ip设置
  [[nodiscard]] dstring getIpMysql() const;
  void setIpMysql(const dstring &value);
  //FTP ip
  [[nodiscard]] dstring getIpFtp() const;
  void setIpFtp(const dstring &value);
  //user设置
  [[nodiscard]] dstring getUser() const;
  [[nodiscard]] dstring getUser_en() const;
  void setUser(const dstring &value);
  void setUser(const QString &value);
  //部门设置
  [[nodiscard]] dstring getDepartment() const;
  [[nodiscard]] QString getDepartmentQ() const;
  void setDepartment(const dstring &value);
  void setDepartment(const QString &value);
  //同步集数设置
  [[nodiscard]] int getSyneps() const;
  void setSyneps(int value);
  //获得freesyn同步软件设置
  [[nodiscard]] dstring getFreeFileSyn() const;
  void setFreeFileSyn(const dstring &value);
  //项目名称设置
  dstring getProjectname();
  [[nodiscard]] std::pair<int,std::string> projectName() const;
  [[nodiscard]] dstringList getAllPrjName() const;
  void setProjectname(const std::string &value);
  void setProjectname(const QString &value);

  //shot根路径
  [[nodiscard]] dpath getShotRoot() const;
  void setShotRoot(const dpath &value);
  //ass根路径
  [[nodiscard]] dpath getAssRoot() const;
  void setAssRoot(const dpath &value);
  //project根路径
  [[nodiscard]] dpath getPrjectRoot() const;
  void setPrjectRoot(const dpath &value);
  //缓存路径
  [[nodiscard]] dpath getCacheRoot() const;

  //doc路径
  [[nodiscard]] dpath getDoc() const;

  void writeDoodleLocalSet();

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
  static dstring toIpPath(const dstring &path);
 private:
  const static dstring settingFileName;
  //MySQL IP地址
  dstring ipMysql;
  //FTP IP地址
  dstring ipFTP;
  //用户名称
  dstring user;
  //部门
  dstring department;
  //同步集数
  int syneps;
  //同步文件的文件运行程序
  dstring freeFileSyn;

  //项目名称
  dstring projectname;
  std::pair<int,std::string> project;
 private:
  //内部属性
  dpathPtr synPath;
  dpathPtr synServer;

  std::map<int,std::string> prjMap;

  dpathPtr shotRoot;
  dpathPtr assRoot;
  dpathPtr prjectRoot;

  dpathPtr cacheRoot;
  dpathPtr doc;
};
inline QString coreSet::getDepartmentQ() const {
  return QString::fromStdString(getDepartment());
}
inline void coreSet::setDepartment(const QString &value) {
  setDepartment(value.toStdString());
}
inline void  coreSet::setUser(const QString &value) {
  setUser(value.toStdString());
}
inline void  coreSet::setSynPathLocale(const QString &syn_path) {
  setSynPathLocale(dpath{syn_path.toStdString()});
}
inline void coreSet::setProjectname(const QString &value) {
  setProjectname(value.toStdString());
}
CORE_NAMESPACE_E

