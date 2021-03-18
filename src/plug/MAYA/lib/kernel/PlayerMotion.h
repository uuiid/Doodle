#pragma once

#include <MotionGlobal.h>

#include <boost/signals2.hpp>

#include <opencv2/opencv.hpp>

#include <QtGui/qimage.h>

namespace doodle::motion::kernel {
class PlayerMotion {
 private:
  //互斥锁
  std::mutex p_mutex;
  //条件变量
  std::condition_variable p_condition;

  std::thread p_thread;

  bool p_thread_stop;

  bool p_stop_player;

  //读取文件的路径
  FSys::path p_video_file;
  std::shared_ptr<cv::VideoCapture> p_video;

  cv::Mat p_frame;

  int p_delay;
  void readFrame();

 public:
  PlayerMotion();

  void startPlayer();
  void stop_Player();

  void setFile(const FSys::path& file);
  boost::signals2::signal<void(QImage&)> fileImage;

  ~PlayerMotion();
};

}  // namespace doodle::motion::kernel