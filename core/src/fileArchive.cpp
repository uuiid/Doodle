#include "fileArchive.h"
#include <QFileInfo>

//设置导入
#include "coreset.h"

//ftp模块导入
#include "src/ftpsession.h"

#include "Logger.h"
#include <boost/filesystem.hpp>
CORE_NAMESPACE_S

fileArchive::fileArchive()
    : p_soureFile(),
      p_cacheFilePath(),
      p_Path(),
      p_state_(state::none) {}

bool fileArchive::update(const dpath &path) {
  dpathList tmp;
  tmp.push_back(path.generic_string());
  return update(tmp);
}

bool fileArchive::update(const dpathList &filelist) {
  if (p_soureFile != filelist) {
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
dpathList fileArchive::down(const dstring &path) {
  return dpathList{};
}

dpathList fileArchive::down() {
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
      dpath file(item);
      if (boost::filesystem::exists(item))
        boost::filesystem::remove(item);
    }
  }
  //复制文件  如果无法复制输出错误
  int i = 0;
  for (auto &&item :p_soureFile) {
    if (item == p_cacheFilePath[i])//如果路径相同就跳过
      continue;

    dpath file(item);
    DOODLE_LOG_INFO << p_soureFile.front().generic_string().c_str()
                    << "-copy->" << p_cacheFilePath.front().generic_string().c_str();
    if (!boost::filesystem::exists(dpath(p_cacheFilePath[i]).parent_path()))
      boost::filesystem::create_directories(dpath(p_cacheFilePath[i]).parent_path());
    boost::filesystem::copy((item), (p_cacheFilePath[i]));
    ++i;
  }
}
bool fileArchive::isInCache() {
  bool has = true;
  if (!p_cacheFilePath.empty()) {
    int i = 0;
    for (auto &&item:p_cacheFilePath) {
      auto fileinfo = dpath(item);

      if (!boost::filesystem::exists(fileinfo.parent_path())) {//首先测试是否存在目录,不存在直接返回
        has &= false;
      } else if (boost::filesystem::exists(fileinfo.parent_path())) {//如果存在就看文件是否存在,  存在就删除
        if (item == p_soureFile[i])
          has &= true;
        else {
          boost::filesystem::remove(fileinfo);
          has &= false;
        }
      } else { has &= false; }
      ++i;
    }
  }
  return has;
}
void fileArchive::_updata(const dpathList &pathList) {
  assert(p_Path.size() == p_cacheFilePath.size());
  coreSet &set = coreSet::getSet();
  DOODLE_LOG_INFO
      << "登录 : "
      << set.getProjectname().c_str()
      << set.getUser_en().c_str()
      << "\n"
      << set.getUser_en().c_str();

  doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session(
      set.getIpFtp(),
      21,
      set.getProjectname() + set.getUser_en(),
      set.getUser_en());

  int i = 0;
  for (auto &&item:p_cacheFilePath) {
    if (!session->upload((item.generic_string()), (p_Path[i].generic_string()))) {
      p_state_ = state::fail;
      DOODLE_LOG_WARN << "无法上传文件" << (item).c_str();
      return;
    }
    ++i;
  }
}
void fileArchive::_down(const dpathList &localPath) {
  assert(p_Path.size() == localPath.size());
  coreSet &set = coreSet::getSet();
  DOODLE_LOG_INFO
      << "登录 : "
      << set.getProjectname().c_str()
      << set.getUser_en().c_str()
      << "\n" << set.getUser_en().c_str();
  doFtp::ftpSessionPtr session = doFtp::ftphandle::getFTP().session(
      set.getIpFtp(),
      21,
      set.getProjectname() + set.getUser_en(),
      set.getUser_en());

  int i = 0;
  for (auto &&item : p_Path) {
    auto k_down_path = dpath(localPath[i]).parent_path();
    if (!boost::filesystem::exists(k_down_path))
      boost::filesystem::create_directories(k_down_path);
    if (!session->down((localPath[i].generic_string()),
                       (item.generic_string()))) {
      DOODLE_LOG_WARN << "无法下载文件" << (item).c_str();
      p_state_ = state::fail;
      return;
    }
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
  coreSet &set = coreSet::getSet();
  for (auto &&item :p_Path) {//这个是下载 要获得p_path服务器路径
    auto path = set.getCacheRoot() / item;
    p_cacheFilePath.push_back(path.generic_string());
  }
  return true;
}

CORE_NAMESPACE_E