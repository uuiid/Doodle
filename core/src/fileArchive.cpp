#include "fileArchive.h"
#include <QFileInfo>

//设置导入
#include "coreset.h"
#include "doException/doException.h"

//ftp模块导入
#include "src/ftpsession.h"

#include "Logger.h"

CORE_NAMESPACE_S

fileArchive::fileArchive() : p_soureFile(),
                             p_cacheFilePath(),
                             p_Path() {}

void fileArchive::update(const QFileInfo &path) {
  QfileInfoVector tmp;
  tmp.append(path);
  update(tmp);
}

void fileArchive::update(const QfileInfoVector &filelist) {
  p_soureFile.clear();
  p_soureFile = filelist;
  _generateFilePath();
  //获得缓存路径
  coreSet &set = coreSet::getCoreSet();
  p_cacheFilePath = QFileInfo(set.getCacheRoot().path() + p_Path);
  if (!isInCache()) {
    copyToCache();
  }
  _updata();
  insertDB();
}
QfileInfoVector fileArchive::down(const QFileInfo &path) {
  return QfileInfoVector();
}

QfileInfoVector fileArchive::down() {
  _generateFilePath();
  //获得缓存路径
  coreSet &set = coreSet::getCoreSet();
  p_cacheFilePath = QFileInfo(set.getCacheRoot().path() + p_Path);

  _down(p_cacheFilePath.filePath());
  return {p_cacheFilePath};
}
//保护功能
void fileArchive::copyToCache() const {
  if (p_cacheFilePath.exists()) //进行检查存在性,  存在即删除
  {
    QFile f;
    f.setFileName(p_cacheFilePath.filePath());
    f.remove();
  }
  //复制文件  如果无法复制就抛出异常
  QFile soureFile_;
  soureFile_.setFileName(p_soureFile[0].filePath());
  DOODLE_LOG_INFO << p_soureFile << "-copy->" << p_cacheFilePath;
  if (!QFile::copy(p_soureFile[0].filePath(), p_cacheFilePath.filePath())) {
    DOODLE_LOG_INFO << "复制文件: " << p_soureFile[0];
    throw doodle_CopyErr(p_soureFile[0].filePath().toStdString());
  }
}
bool fileArchive::isInCache() {

//  if (!p_soureFile[0].exists())
//    throw doodle_notFile(p_soureFile[0].filePath().toStdString());

  if (p_cacheFilePath.exists()) //不存在缓存目录
  {
    QFile(p_cacheFilePath.path()).remove();
  }
  DOODLE_LOG_INFO << QDir(p_cacheFilePath.path());
  if (!QDir(p_cacheFilePath.path()).exists()) {
    QDir().mkpath(p_cacheFilePath.path());
  }
  return false;
}
void fileArchive::_updata() {
  coreSet &set = coreSet::getCoreSet();
  DOODLE_LOG_INFO << "登录 : " << set.getProjectname() + set.getUser_en() << "\n" << set.getUser_en();
  doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session(
      set.getIpFtp(),
      21,
      set.getProjectname() + set.getUser_en(),
      set.getUser_en());
  if (!session->upload(p_cacheFilePath.absoluteFilePath(), p_Path))
    throw doodle_upload_error(p_cacheFilePath.absoluteFilePath().toStdString());
}
void fileArchive::_down(const QString &localPath) {
  coreSet &set = coreSet::getCoreSet();
  DOODLE_LOG_INFO << "登录 : " << set.getProjectname() + set.getUser_en() << "\n" << set.getUser_en();
  doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session(
      set.getIpFtp(),
      21,
      set.getProjectname() + set.getUser_en(),
      set.getUser_en());
  auto k_down_path = QFileInfo(localPath).path();
  if(!QDir(k_down_path).exists())
    QDir(k_down_path).mkpath(k_down_path);

  if (!session->down(localPath, p_soureFile[0].filePath()))
    DOODLE_LOG_WARN << "无法下载文件" << p_soureFile[0].filePath();
}

CORE_NAMESPACE_E