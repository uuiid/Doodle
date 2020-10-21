#pragma once

#include "core_global.h"
// #include <QFileInfo>
// #include <QVector>

CORE_NAMESPACE_S

class CORE_EXPORT fileArchive {
 public:
  fileArchive();
  virtual ~fileArchive() = default;;

  virtual void update(const QFileInfo &path);
  virtual void update(const QfileInfoVector &filelist);
  virtual QfileInfoVector down(const QFileInfo &path);

 protected:
  //复制到和缓存文件夹
  virtual void copyToCache() const;
  //判断是否在缓存文件夹
  virtual bool isInCache();
  //提交到数据库
  virtual void insertDB() = 0;
  //上传文件
  virtual void _updata();
  //组合出服务器路径路径
  virtual void _generateFilePath() = 0;

 protected:
  //复制的数据来源
  QfileInfoVector p_soureFile;
  //主要属性  必须进行初始化
  fileSqlInfoPtr p_fileInfo;
  //缓存路径
  QFileInfo p_cacheFilePath;
  //这个是服务器路径
  QString p_Path;
};

CORE_NAMESPACE_E
