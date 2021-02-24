/*
 * @Author: your name
 * @Date: 2020-09-27 14:36:04
 * @LastEditTime: 2020-12-01 14:00:03
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\src\fileArchive\movieArchive.cpp
 */
#include "movieArchive.h"
#include <corelib/shots/shottype.h>
#include <corelib/exeWrap/ffmpegWrap.h>
#include <corelib/shots/shotfilesqlinfo.h>
#include <corelib/fileArchive/fileArchive.h>

#include <loggerlib/Logger.h>

#include <boost/filesystem.hpp>

#include <utility>
DOODLE_NAMESPACE_S
movieArchive::movieArchive(fileSqlInfoPtr shot_info_ptr)
    : fileArchive(),
      p_info_ptr_(std::move(shot_info_ptr)) {
}
void movieArchive::insertDB() {
  p_info_ptr_->setFileList(p_ServerPath);

  if (p_info_ptr_->getInfoP().empty())
    p_info_ptr_->setInfoP("拍屏文件");
}
void movieArchive::imp_generateFilePath() {
  if (!p_soureFile.empty())
    p_ServerPath.push_back(p_info_ptr_->generatePath("movie", ".mp4").generic_string());
  else if (!p_info_ptr_->getFileList().empty())
    p_ServerPath.push_back(p_info_ptr_->getFileList()[0].generic_string());
}
bool movieArchive::makeMovie(const fileSys::path &imageFolder) {
  auto ffmpeg = std::make_unique<ffmpegWrap>();
  auto path   = fileSys::path(imageFolder);
  dpathList list;
  for (auto &&item : boost::filesystem::directory_iterator(path)) {
    if (item.path().extension() == ".png" || item.path().extension() == ".jpg" || item.path().extension() == ".tga" || item.path().extension() == ".exr") {
      list.push_back(item.path().string());
    }
  }
  return ffmpeg->imageToVideo(list,
                              p_cacheFilePath.front(),
                              boost::filesystem::basename(p_cacheFilePath.front()));
}

bool movieArchive::convertMovie(const fileSys::path &moviePath) {
  auto ffmpeg = std::make_unique<ffmpegWrap>();
  return ffmpeg->convertToVideo(
      moviePath,
      p_cacheFilePath.front(),
      boost::filesystem::basename(p_cacheFilePath.front()));
}
//bool movieArchive::update(const std::vector<QString> &filelist) {
//  dpathList kdpath_list;
//  for (const auto &item : filelist) {
//    kdpath_list.push_back(item.toStdString());
//  }
//  return update(kdpath_list);
//}
bool movieArchive::update(const dpathList &filelist) {
  p_soureFile = filelist;
  DOODLE_LOG_INFO(filelist.front().c_str());
  setInfoAttr();
  imp_generateFilePath();
  generateCachePath();

  bool isok = false;
  if (filelist.size() == 1) {
    if (boost::filesystem::is_directory(filelist.front()))  //QFileInfo().isDir()
      isok = makeMovie(filelist.front());
    else if (boost::filesystem::is_regular_file(filelist.front()))  //QFileInfo().isFile()
      isok = convertMovie(filelist.front());
  } else {
    return false;
  }

  if (isok) {
    p_state_ = state::success;
    imp_updata(p_cacheFilePath);
    insertDB();
  } else
    p_state_ = state::fail;

  return isok;
}

DOODLE_NAMESPACE_E
