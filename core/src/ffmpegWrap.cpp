//
// Created by teXiao on 2020/10/28.
//

#include "ffmpegWrap.h"

#include "Logger.h"

#include <QTemporaryFile>
#include <QtCore/QDir>

CORE_NAMESPACE_S
ffmpegWrap::ffmpegWrap(QString path)
    : p_path_(std::move(path)),
      p_file_() {
  p_file_ = std::make_shared<QTemporaryFile>();
}
bool ffmpegWrap::imageToVideo(const std::vector<QString> &image_path,
                              const QString &videoPath,
                              const QString &subtitles) const {
  p_file_->open();
  for (auto &&item :image_path) {
    p_file_->write(QString("'%1'\n").arg(item).toStdString().c_str());
  }
  p_file_->close();
  //不纯在路径创建一下
  auto dir = QDir(QFileInfo(videoPath).path());
  if (!dir.exists()) dir.mkpath(dir.path());

  auto comm = QString(R"("%1" -r 25 -f concat -safe 0 -i %2 )"
                      R"(-filter_complex "drawtext=text='%3' : fontcolor=0xc62d1d: fontsize=44: x=10:y=10: shadowx=3: shadowy=3")"
                      R"( -c:v libx264 -pix_fmt yuv420p -s 1920*1080 %4 ")")
      .arg(p_path_)
      .arg(p_file_->fileName())
      .arg(subtitles)
      .arg(videoPath);
  DOODLE_LOG_INFO << "转换视频命令 : " << comm << "\n转换视频位置:" << videoPath;

//  std::system(comm.toStdString().c_str());
  return true;
}

CORE_NAMESPACE_E
