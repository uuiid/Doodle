#include "movieArchive.h"
#include "filetype.h"

#include "ffmpegWrap.h"
#include "Logger.h"

#include <QtCore/QDir>

#include "shotfilesqlinfo.h"
CORE_NAMESPACE_S
movieArchive::movieArchive(doCore::shotInfoPtr &shot_info_ptr)
    : fileArchive(),
      p_info_ptr_(shot_info_ptr) {

}
void movieArchive::insertDB() {
  p_info_ptr_->setFileList(p_Path);

  p_info_ptr_->insert();
}
void movieArchive::_generateFilePath() {
  if (!p_soureFile.empty())
    p_Path.push_back(p_info_ptr_->generatePath("movie", "mp4"));
  else if (!p_info_ptr_->getFileList().empty())
    p_Path.push_back(p_info_ptr_->getFileList()[0].filePath());
}
bool movieArchive::makeMovie(const QString &imageFolder) {

  auto dir = QDir(imageFolder);
  auto image_list = dir.entryInfoList({"*.png", "*.jpg", "*.tga", "*.exr"}, QDir::Files, QDir::Name);

  auto ffmpeg = std::make_unique<ffmpegWrap>(findFFmpeg().toStdString() + "/ffmpeg.exe");
  std::vector<QString> list;
  for (auto &&item :image_list.toStdList()) list.push_back(item.absoluteFilePath());

  return ffmpeg->imageToVideo(list, p_cacheFilePath.front(), QFileInfo(p_cacheFilePath.front()).baseName());
}

bool movieArchive::convertMovie(const QString &moviePath) {
  auto ffmpeg = std::make_unique<ffmpegWrap>(findFFmpeg().toStdString() + "/ffmpeg.exe");
  return ffmpeg->convertToVideo(
      moviePath.toStdString(),
      p_cacheFilePath.front().toStdString(),
      QFileInfo(p_cacheFilePath.front()).baseName().toStdString()
      );
}
bool movieArchive::update(const stringList &filelist) {
  p_soureFile = filelist;
  DOODLE_LOG_INFO << filelist;

  const fileTypePtr &kType = p_info_ptr_->findFileType("movie");
  auto version = shotFileSqlInfo::getAll(kType).size();
  p_info_ptr_->setVersionP(version + 1);
  p_info_ptr_->setFileType(kType);
  p_info_ptr_->setInfoP("拍屏");
  _generateFilePath();
  generateCachePath();


  bool isok = false;
  if (filelist.size() == 1) {
    if (QFileInfo(filelist.front()).isDir())
      isok = makeMovie(filelist.front());
    else if (QFileInfo(filelist.front()).isFile())
      isok = convertMovie(filelist.front());
  }else{
    return false;
  }

  if (isok) {
    p_state_ = state::success;
    _updata(p_cacheFilePath);
    insertDB();
  } else
    p_state_ = state::fail;

  return isok;
}
QString movieArchive::findFFmpeg() const {
  auto ffmpeg_exe = QDir::current();
  ffmpeg_exe.cdUp();
  ffmpeg_exe.cd(DOODLE_FFMPEG_PATH);
  DOODLE_LOG_INFO << "找到ffmpeg" << ffmpeg_exe.absolutePath() << "\n" << ffmpeg_exe;
  return ffmpeg_exe.path();
}

CORE_NAMESPACE_E
