#include "fileArchive.h"
#include <QFileInfo>

//设置导入
#include "coreset.h"

//ftp模块导入
#include "src/ftpsession.h"

#include "Logger.h"

CORE_NAMESPACE_S

fileArchive::fileArchive()
    : p_soureFile(),
      p_cacheFilePath(),
      p_Path(),
      p_state_(state::none){}

bool fileArchive::update(const QFileInfo &path) {
  stringList tmp;
  tmp.push_back(path.filePath());
  return update(tmp);
}

bool fileArchive::update(const stringList &filelist) {
  if(p_soureFile != filelist) {
    p_soureFile.clear();
    p_soureFile = filelist;
  }
  _generateFilePath();
  //获得缓存路径
  generateCachePath();

  if (!isInCache()) {
    copyToCache();
  }
  _updata(p_cacheFilePath);
  insertDB();
  return true;
}
stringList fileArchive::down(const QString &path) {
  return stringList{};
}

stringList fileArchive::down() {
  _generateFilePath();
  //获得缓存路径
  generateCachePath();
  _down(p_cacheFilePath);
  return p_cacheFilePath;
}
//保护功能
void fileArchive::copyToCache() const {
  assert(p_soureFile.size() == p_cacheFilePath.size());
  if (!p_cacheFilePath.empty()) //进行检查存在性,  存在即删除
  {
    for (auto &&item :p_cacheFilePath) {
      QFile file(item);
      if (file.exists())
        file.remove();
    }
  }
  //复制文件  如果无法复制输出错误
  int i = 0;
  for (auto &&item :p_soureFile) {
    if (item == p_cacheFilePath[i])//如果路径相同就跳过
      continue;

    QFile file(item);
    DOODLE_LOG_INFO << p_soureFile << "-copy->" << p_cacheFilePath;
    if (QFile::copy((item),
                    (p_cacheFilePath[i])))
      DOODLE_LOG_WARN << "复制文件: " << p_soureFile;
    ++i;
  }
}
bool fileArchive::isInCache() {
  bool has = true;
  if (!p_cacheFilePath.empty()) {
    int i = 0;
    for (auto &&item:p_cacheFilePath) {
      auto fileinfo = QFileInfo((item));

      if (!QFileInfo(fileinfo.path()).exists()) {//首先测试是否存在目录,不存在直接返回
        has &= false;
      } else if (fileinfo.exists()) {//如果存在就看文件是否存在,  存在就删除
        if (item == p_soureFile[i])
          has &= true;
        else {
          QFile(fileinfo.filePath()).remove();
          has &= false;
        }
      }else
        has &= false;
      ++i;
    }
  }
  return has;
}
void fileArchive::_updata(const stringList &pathList) {
  assert(p_Path.size() == p_cacheFilePath.size());
  coreSet &set = coreSet::getCoreSet();
  DOODLE_LOG_INFO << "登录 : " << set.getProjectname() + set.getUser_en() << "\n" << set.getUser_en();
  doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session(
      set.getIpFtp(),
      21,
      set.getProjectname() + set.getUser_en(),
      set.getUser_en());

  int i = 0;
  for (auto &&item:p_cacheFilePath) {
    if (!session->upload((item), (p_Path[i])))
      DOODLE_LOG_WARN << "无法上传文件" << (item);
    ++i;
  }
}
void fileArchive::_down(const stringList &localPath) {
  assert(p_Path.size() == localPath.size());
  coreSet &set = coreSet::getCoreSet();
  DOODLE_LOG_INFO << "登录 : " << set.getProjectname() + set.getUser_en() << "\n" << set.getUser_en();
  doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session(
      set.getIpFtp(),
      21,
      set.getProjectname() + set.getUser_en(),
      set.getUser_en());

  int i = 0;
  for (auto &&item : p_Path) {
    auto k_down_path = QFileInfo((localPath[i])).path();
    if (!QDir(k_down_path).exists())
      QDir(k_down_path).mkpath(k_down_path);
    if (!session->down((localPath[i]),
                       (item)))
      DOODLE_LOG_WARN << "无法下载文件" << (item);
  }
}
bool fileArchive::update() {
  insertDB();
  return true;
}
fileArchive::state fileArchive::isState() const {
  return p_state_;
}
bool fileArchive::generateCachePath() {
  //获得缓存路径
  p_cacheFilePath.clear();
  coreSet &set = coreSet::getCoreSet();
  for (auto &&item :p_Path) {//这个是下载 要获得p_path服务器路径
    auto path = set.getCacheRoot().path().append((item));
    p_cacheFilePath.push_back(path);
  }
  return true;
}

CORE_NAMESPACE_E