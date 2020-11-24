#pragma once

#include "core_global.h"
// #include <QFileInfo>
// #include <QVector>
#include <boost/filesystem.hpp>
#include <ftp_global.h>
CORE_NAMESPACE_S

class CORE_API fileArchive{
 public:
  fileArchive();
  ~fileArchive() = default;;
  enum class state{
    none = 0,
    success = 1,
    fail = 2,
  };

  [[nodiscard]] state isState() const;

  virtual bool update(const dpath &path);
  virtual bool update(const dpathList &filelist);
  virtual bool update();
  virtual dpath down(const dstring &path);
  virtual dpath down();

 protected:
  //复制到和缓存文件夹
  virtual void copyToCache() const;
  //判断是否在缓存文件夹
  //我们认为缓存目录是一样的  不存在两个不同的目录  只有文件名称不同
  virtual bool isInCache();
  //生成cachepath
  virtual bool generateCachePath();


  //提交到数据库
  virtual void insertDB() = 0;
  //上传文件
  virtual void _updata(const dpathList &pathList);
  //组合需要的路径  包括来源  缓存和服务器路径
  virtual void _generateFilePath() = 0;
  //下载文件
  virtual void _down(const dpath &localPath);

  bool copy(const dpath& sourePath,const  dpath& trange_path) const noexcept;
  void login();
  static dstring findFFmpeg() ;
 protected:
  //复制的数据来源(本地)
  dpathList p_soureFile;
  //缓存路径
  dpathList p_cacheFilePath;
  //这个是服务器路径(服务器)
  dpathList p_Path;
  //状态
  state p_state_;

  //ftp
  doFtp::ftpSessionPtr p_session_;
};

CORE_NAMESPACE_E
