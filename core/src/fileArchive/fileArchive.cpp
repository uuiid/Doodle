#include "src/shots/shotfilesqlinfo.h"
#include <utility>
#include <QtCore/QDir>
#include "src/exeWrap/ffmpegWrap.h"
#include "src/shots/shottype.h"
#include "movieArchive.h"
#include "fileArchive.h"
#include <QFileInfo>

//设置导入
#include "src/core/coreset.h"

// ftp模块导入
#include "src/ftpsession.h"

#include "Logger.h"
#include <boost/filesystem.hpp>
#include <regex>

DOODLE_NAMESPACE_S

fileArchive::fileArchive()
    : p_soureFile(),
      p_cacheFilePath(),
      p_ServerPath(),
      p_state_(state::none) {}

bool fileArchive::update(const dpath &path) {
  updateChanged(1);

  dpathList tmp;
  tmp.push_back(path.generic_string());
  return update(tmp);
}

bool fileArchive::update(const dpathList &filelist) {
  infoChanged("上传开始");
  if (p_soureFile != filelist) {
    p_soureFile.clear();
    p_soureFile = filelist;
  }

  infoChanged("生成路径中");
  _generateFilePath();

  //获得缓存路径
  generateCachePath();

  if (!isInCache()) {
    copyToCache();
  }

  if (useUpdataCheck())
    updataCheck();

  _updata(p_cacheFilePath);
  insertDB();
  // updateChanged(100);
  return true;
}
dpath fileArchive::down(const dstring &path) {
  _generateFilePath();
  //获得缓存路径
  generateCachePath();
  _down(path);
  if (useDownloadCheck())
    downloadCheck();

  return {path};
}

dpath fileArchive::down() {
  _generateFilePath();
  //获得缓存路径
  generateCachePath();
  _down(p_cacheFilePath.front().parent_path());
  return p_cacheFilePath.front();
}

bool fileArchive::useUpdataCheck() const {
  return false;
}

bool fileArchive::updataCheck() const {
  return false;
}

bool fileArchive::useDownloadCheck() const {
  return false;
}

bool fileArchive::downloadCheck() const {
  return false;
}

//保护功能
void fileArchive::copyToCache() const {
  assert(p_soureFile.size() == p_cacheFilePath.size());

  updateChanged(10);
  //复制文件  如果无法复制输出错误
  infoChanged("复制文件到缓存位置 ");
  for (int index = 0; index < p_soureFile.size(); ++index) {
    if (p_soureFile[index] == p_cacheFilePath[index])
      continue;  //如果路径相同就跳过

    DOODLE_LOG_INFO(p_soureFile[index].generic_string().c_str()
                    << "-copy->"
                    << p_cacheFilePath[index].generic_string().c_str());

    infoChanged(p_cacheFilePath[index].generic_string());

    if (!boost::filesystem::exists(p_cacheFilePath[index].parent_path()))
      boost::filesystem::create_directories(
          p_cacheFilePath[index].parent_path());
    boost::filesystem::copy(p_soureFile[index], p_cacheFilePath[index]);
  }
}

bool fileArchive::isInCache() {
  bool has = true;
  if (!p_cacheFilePath.empty()) {
    for (int index = 0; index < p_cacheFilePath.size(); ++index) {
      if (!boost::filesystem::exists(p_cacheFilePath[index].parent_path())) {
        //首先测试是否存在目录,不存在直接返回
        has &= false;
      } else {
        //如果存在就看文件是否存在,
        //存在就删除(boost::filesystem::exists(p_cacheFilePath[index].parent_path()))
        if (p_cacheFilePath[index] == p_soureFile[index])
          has &= true;
        else {
          boost::filesystem::remove(p_cacheFilePath[index]);
          has &= false;
        }
      }
    }
  }
  updateChanged(1);
  infoChanged(std::string{"测试文件是否在缓存位置上 : "}.append(has ? "true" : "false"));
  return has;
}

void fileArchive::_updata(const dpathList &pathList) {
  assert(p_ServerPath.size() == p_cacheFilePath.size());
  coreSet &set = coreSet::getSet();

  auto &session = doSystem::DfileSyntem::get();

  const auto k_size = p_cacheFilePath.size();
  for (size_t i = 0; i < k_size; ++i) {
    updateChanged(k_size / 50);
    if (p_cacheFilePath[i] != set.getPrjectRoot() / p_ServerPath[i]) {
      if (!session.upload(p_cacheFilePath[i], p_ServerPath[i])) {
        p_state_ = state::fail;
        DOODLE_LOG_WARN("无法上传文件" << p_cacheFilePath[i].c_str());
        return;
      }
    } else {
      infoChanged("已经在服务器上只需要记录,  不需要上传");
      DOODLE_LOG_WARN("已经在服务器上只需要记录,  不需要上传");
    }
  }
}
void fileArchive::_down(const dpath &localPath) {
  auto &session = doSystem::DfileSyntem::get();
  infoChanged("开始下载");

  for (auto &&item : p_ServerPath) {
    if (!boost::filesystem::exists(localPath))
      boost::filesystem::create_directories(localPath);
    if (!session.down(localPath / item.filename(), item)) {
      infoChanged(std::string{"无法下载文件"}.append(item.generic_string()));
      DOODLE_LOG_WARN("无法下载文件" << item.c_str());

      p_state_ = state::fail;
      continue;
    }
  }
}

bool fileArchive::isServerzinsideDir(const dpath &localPath) {
  auto &set        = coreSet::getSet();
  auto projectRoot = set.getPrjectRoot();
  if (projectRoot.has_root_name() && localPath.has_root_name()) {
    infoChanged("判断为服务器路径");
    return projectRoot.root_name() == localPath.root_name();
  } else {
    infoChanged("判断为本地路径");
    return false;
  }
}
bool fileArchive::update() {
  infoChanged("直接插入数据库， 不复制路径");
  DOODLE_LOG_INFO("直接插入数据库， 不复制路径");

  insertDB();
  updateChanged(100);
  return true;
}
fileArchive::state fileArchive::isState() const { return p_state_; }

bool fileArchive::generateCachePath() {
  infoChanged("获得缓存路径");
  updateChanged(5);

  //获得缓存路径
  p_cacheFilePath.clear();
  coreSet &set = coreSet::getSet();
  for (auto &&item : p_ServerPath) {  //这个是下载 要获得p_path服务器路径
    auto path = set.getCacheRoot() / item;
    p_cacheFilePath.push_back(path.generic_string());
  }
  return true;
}

DOODLE_NAMESPACE_E