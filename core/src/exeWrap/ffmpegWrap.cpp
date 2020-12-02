//
// Created by teXiao on 2020/10/28.
//

#include "ffmpegWrap.h"

#include "Logger.h"

#include <boost/process.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <iostream>

CORE_NAMESPACE_S
ffmpegWrap::ffmpegWrap()
    : p_tmp_file_() {
  p_tmp_file_ = std::make_shared<boost::filesystem::path>(
      boost::filesystem::temp_directory_path() /
      boost::filesystem::unique_path().filename());
}
bool ffmpegWrap::imageToVideo(const dpathList &image_path,
                              const dpath &videoPath,
                              const std::string &subtitles) const {
  boost::filesystem::ofstream fileOpen(*p_tmp_file_);

  for (auto &&item : image_path) {
    fileOpen << boost::format("file '%1%'\n") % item.generic_string();
  }
  fileOpen.close();
  //如果不存在就创建
  if (!boost::filesystem::exists(videoPath.parent_path()))
    boost::filesystem::create_directories(videoPath.parent_path());
  if (boost::filesystem::exists(videoPath))
    boost::filesystem::remove(videoPath);

  //  stringList com_arg;
  auto com_arg = boost::format(
                     "ffmpeg.exe -r 25 -f concat -safe 0 -i %1% "
                     "-filter_complex "
                     "\"drawtext=text='%2%':fontcolor=0xc62d1d:fontsize=44:x="
                     "10:y=10:shadowx=3:shadowy=3\" "
                     "-c:v libx264 -pix_fmt yuv420p -s 1920*1080 %3%") %
                 p_tmp_file_->generic_string() % subtitles % videoPath;

  DOODLE_LOG_INFO << QString::fromStdString(com_arg.str());

  runFFmpeg(com_arg.str());

  return boost::filesystem::exists(videoPath);
}

bool ffmpegWrap::imageToVideo(const std::vector<QString> &image_path,
                              const QString &videoPath,
                              const QString &subtitles) const {
  std::vector<dpath> k_image_path;
  k_image_path.clear();
  for (auto &&x : image_path) k_image_path.emplace_back(x.toStdString());
  return imageToVideo(k_image_path, videoPath.toStdString(),
                      subtitles.toStdString());
}
bool ffmpegWrap::convertToVideo(const dpath &in_videoPath,
                                const dpath &out_videoPath,
                                const std::string &subtitles) const {
  boost::filesystem::path k_out_path(out_videoPath);
  if (!boost::filesystem::exists(k_out_path.parent_path()))
    boost::filesystem::create_directories(k_out_path.parent_path());
  if (boost::filesystem::exists(k_out_path))
    boost::filesystem::remove(k_out_path);

  auto com_arg =
      boost::format(
          "ffmpeg.exe -i %1% -vcodec h264"
          " -filter_complex \"drawtext=text='%2%':"
          "fontcolor=0xc62d1d:fontsize=44:x=10:y=10:shadowx=3:shadowy=3\" "
          "-acodec mp2 -s 1920*1080 %3%") %
      in_videoPath % subtitles % out_videoPath;
  DOODLE_LOG_INFO << QString::fromStdString(com_arg.str());

  runFFmpeg(com_arg.str());

  return boost::filesystem::exists(k_out_path);
}
bool ffmpegWrap::connectVideo(const dpathList &in_videoPath,
                              const dpath &out_videoPath) {
  boost::filesystem::ofstream fileOpen(*p_tmp_file_);

  for (auto &&item : in_videoPath) {
    fileOpen << boost::format("file '%1%'\n") % item.generic_string();
  }
  fileOpen.close();

  //如果不存在就创建
  if (!boost::filesystem::exists(out_videoPath.parent_path()))
    boost::filesystem::create_directories(out_videoPath.parent_path());
  if (boost::filesystem::exists(out_videoPath))
    boost::filesystem::remove(out_videoPath);

  //  stringList com_arg;
  //"-filter_complex
  //\"drawtext=text='%2%':fontcolor=0xc62d1d:fontsize=44:x=10:y=10:shadowx=3:shadowy=3\"
  //"
  auto com_arg = boost::format(
                     "ffmpeg.exe -r 25 -f concat -safe 0 -i \"%1%\" "
                     "-c:v libx264 -pix_fmt yuv420p -s 1920*1080 -movflags "
                     "+faststart %2%") %
                 p_tmp_file_->generic_path().string() %
                 out_videoPath.generic_path().string();
  DOODLE_LOG_INFO << QString::fromStdString(com_arg.str());

  auto env = boost::this_process::environment();

  runFFmpeg(com_arg.str());

  return boost::filesystem::exists(out_videoPath);
}

bool ffmpegWrap::runFFmpeg(const std::string &command) const {
  for (const auto &item : boost::this_process::environment()["PATH"].to_vector()) {
    DOODLE_LOG_INFO << item.c_str();
  }
  STARTUPINFO si{};
  PROCESS_INFORMATION pi{};
  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(pi));

  try {
    //使用windowsIPA创建子进程
    CreateProcess(
        NULL,
        (char *)command.c_str(),
        NULL,
        NULL,
        false,
        0,  //CREATE_NEW_CONSOLE
        NULL,
        NULL,  //R"(C:\Program Files\Autodesk\Maya2018\bin\)"
        &si,
        &pi);
    // boost::process::system(command.c_str(), env);
  } catch (const boost::process::process_error &err) {
    DOODLE_LOG_WARN << err.what() << err.code().message().c_str();
    return false;
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return true;
}

CORE_NAMESPACE_E
