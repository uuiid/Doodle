#pragma once

#include "core_global.h"
// #include <QFileInfo>
// #include <QVector>

CORE_NAMESPACE_S

class CORE_EXPORT fileArchive{
 public:
  fileArchive();
  ~fileArchive() = default;;
  enum class state{
    none = 0,
    success = 1,
    fail = 2,
  };

  [[nodiscard]] state isState() const;

  virtual bool update(const QFileInfo &path);
  virtual bool update(const stringList &filelist);
  virtual bool update();
  virtual stringList down(const QString &path);
  virtual stringList down();

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
  virtual void _updata(const stringList &pathList);
  //组合需要的路径  包括来源  缓存和服务器路径
  virtual void _generateFilePath() = 0;
  //下载文件
  virtual void _down(const stringList &localPath);
 protected:
  //复制的数据来源(本地)
  stringList p_soureFile;
  //缓存路径
  stringList p_cacheFilePath;
  //这个是服务器路径(服务器)
  stringList p_Path;

  //状态
  state p_state_;
};

CORE_NAMESPACE_E
