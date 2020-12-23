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

CORE_NAMESPACE_S

fileArchive::fileArchive()
    : p_custom_path(),
      p_soureFile(),
      p_cacheFilePath(),
      p_ServerPath(),
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

  if (!p_custom_path.empty() || p_soureFile.size() == p_custom_path.size()) {
    p_ServerPath = p_custom_path;
  }

  //获得缓存路径
  generateCachePath();

  if (!isInCache()) {
    copyToCache();
  }

  if (useUpdataCheck())
    updataCheck();

  _updata(p_cacheFilePath);
  insertDB();
  return true;
}
dpath fileArchive::down(const dstring &path) {
  _generateFilePath();
  //获得缓存路径
  generateCachePath();
  _down(path);
  if (useDowndataCheck())
    downdataCheck();

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

bool fileArchive::useDowndataCheck() const {
  return false;
}

bool fileArchive::downdataCheck() const {
  return false;
}

void fileArchive::setUseCustomPath(const dpathList &custom_path) {
  if (custom_path.size() == p_soureFile.size())
    p_custom_path = custom_path;
  else {
    DOODLE_LOG_ERROR("not custom path " << custom_path.front());
    throw std::runtime_error("not custom path");
  }
}

//保护功能
void fileArchive::copyToCache() const {
  assert(p_soureFile.size() == p_cacheFilePath.size());

  //复制文件  如果无法复制输出错误
  for (int index = 0; index < p_soureFile.size(); ++index) {
    if (p_soureFile[index] == p_cacheFilePath[index])
      continue;  //如果路径相同就跳过

    DOODLE_LOG_INFO(p_soureFile[index].generic_string().c_str()
                    << "-copy->"
                    << p_cacheFilePath[index].generic_string().c_str());
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
      } else {  //如果存在就看文件是否存在,
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
  return has;
}
void fileArchive::_updata(const dpathList &pathList) {
  assert(p_ServerPath.size() == p_cacheFilePath.size());
  coreSet &set = coreSet::getSet();

  auto &session = doSystem::DfileSyntem::get();

  //使用ftp上传
  for (size_t i = 0; i < p_cacheFilePath.size(); ++i) {
    if (p_cacheFilePath[i] != p_ServerPath[i]) {
      if (!session.upload(p_cacheFilePath[i], p_ServerPath[i])) {
        p_state_ = state::fail;
        DOODLE_LOG_WARN("无法上传文件" << p_cacheFilePath[i].c_str());
        return;
      }
    } else {
      DOODLE_LOG_WARN("已经在服务器上只需要记录,  不需要上传");
    }
  }
}
void fileArchive::_down(const dpath &localPath) {
  auto &session = doSystem::DfileSyntem::get();
  for (auto &&item : p_ServerPath) {
    if (!boost::filesystem::exists(localPath))
      boost::filesystem::create_directories(localPath);
    if (!session.down(localPath / item.filename(), item)) {
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
    return projectRoot.root_name() == localPath.root_name();
  } else {
    return false;
  }
}
bool fileArchive::update() {
  insertDB();
  return true;
}
fileArchive::state fileArchive::isState() const { return p_state_; }

bool fileArchive::generateCachePath() {
  //获得缓存路径
  p_cacheFilePath.clear();
  coreSet &set = coreSet::getSet();
  for (auto &&item : p_ServerPath) {  //这个是下载 要获得p_path服务器路径
    auto path = set.getCacheRoot() / item;
    p_cacheFilePath.push_back(path.generic_string());
  }
  return true;
}

CORE_NAMESPACE_E