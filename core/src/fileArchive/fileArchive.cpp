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

//ftp模块导入
#include "src/ftpsession.h"

#include "Logger.h"
#include <boost/filesystem.hpp>
#include <regex>

CORE_NAMESPACE_S

dstring doCore::fileArchive::findFFmpeg() {
  auto ffmpeg_exe = QDir::current();
  ffmpeg_exe.cdUp();
  ffmpeg_exe.cd(DOODLE_FFMPEG_PATH);
  DOODLE_LOG_INFO << "找到ffmpeg" << ffmpeg_exe.absolutePath() << "\n" << ffmpeg_exe;
  return ffmpeg_exe.path().toStdString();
}
fileArchive::fileArchive()
    : p_soureFile(),
      p_cacheFilePath(),
      p_Path(),
      p_session_(),
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
dpath fileArchive::down(const dstring &path) {
  _generateFilePath();
  //获得缓存路径
  generateCachePath();
  _down(path);
  return {path};
}

dpath fileArchive::down() {
  _generateFilePath();
  //获得缓存路径
  generateCachePath();
  _down(p_cacheFilePath.front().parent_path());
  return p_cacheFilePath.front();
}
//保护功能
void fileArchive::copyToCache() const {
  assert(p_soureFile.size() == p_cacheFilePath.size());

  //复制文件  如果无法复制输出错误
  for (int index = 0; index < p_soureFile.size(); ++index) {
    if (p_soureFile[index] == p_cacheFilePath[index])
      continue;//如果路径相同就跳过

    DOODLE_LOG_INFO << p_soureFile[index].generic_string().c_str()
                    << "-copy->" << p_cacheFilePath[index].generic_string().c_str();
    if (!boost::filesystem::exists(p_cacheFilePath[index].parent_path()))
      boost::filesystem::create_directories(p_cacheFilePath[index].parent_path());
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
      } else {        //如果存在就看文件是否存在,  存在就删除(boost::filesystem::exists(p_cacheFilePath[index].parent_path()))
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
  assert(p_Path.size() == p_cacheFilePath.size());
  coreSet &set = coreSet::getSet();
  DOODLE_LOG_INFO
      << "登录 : "
      << set.getProjectname().c_str()
      << set.getUser_en().c_str()
      << "\n"
      << set.getUser_en().c_str();

  auto session = doSystem::DfileSyntem::getFTP().session(
      set.getIpFtp(),
      21,
      set.getProjectname() + set.getUser_en(),
      set.getUser_en()
  );

  int i = 0;
  for (auto &&item:p_cacheFilePath) {
    if (
        !session->upload((item.generic_string()), (p_Path[i].generic_string()), true)
        ) {
      p_state_ = state::fail;
      DOODLE_LOG_WARN << "无法上传文件" << (item).c_str();
      return;
    }
    ++i;
  }
}
void fileArchive::_down(const dpath &localPath) {
  if (!p_session_) {
    login();
  }

  for (auto &&item : p_Path) {
    if (!boost::filesystem::exists(localPath))
      boost::filesystem::create_directories(localPath);
    if (!p_session_->down((localPath / item.filename()).generic_string(),
                       item.generic_string())) {
      DOODLE_LOG_WARN << "无法下载文件" << item.c_str();
      p_state_ = state::fail;
      continue;
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
void fileArchive::login() {
  coreSet &set = coreSet::getSet();
  DOODLE_LOG_INFO
      << "登录 : "
      << set.getProjectname().c_str()
      << set.getUser_en().c_str()
      << "\n" << set.getUser_en().c_str();
  p_session_ = doSystem::DfileSyntem::getFTP().session(
      set.getIpFtp(),
      21,
      set.getProjectname() + set.getUser_en(),
      set.getUser_en());
}

CORE_NAMESPACE_E