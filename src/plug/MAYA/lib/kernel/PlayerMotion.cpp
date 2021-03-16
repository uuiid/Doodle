#include <lib/kernel/PlayerMotion.h>

#include <lib/kernel/Exception.h>

// #include <opencv2/opencv.hpp>
#include <chrono>

namespace doodle::motion::kernel {

void PlayerMotion::readFrame() {
  std::unique_lock<std::mutex> lock(p_mutex);
  if (this->p_stop_player)
    return;
  if (!this->p_video->isOpened())
    return;

  if (!this->p_video->read(p_frame)) {
    p_video->set(cv::CAP_PROP_POS_FRAMES, 0);
    return;
  }

  auto image = QImage{};
  auto frame = cv::Mat{};
  cv::cvtColor(p_frame, frame, cv::COLOR_BGR2RGB);
  auto k_image = QImage{
      (const unsigned char *)frame.data,
      frame.cols,
      frame.rows,
      (int)frame.step,
      QImage::Format_RGB888};
  // image = k_image.copy();
  fileImage(k_image.copy());
  // if (p_frame.channels() == 3) {
  // } else {
  //   auto k_image = QImage{
  //       (const unsigned char *)p_frame.data,
  //       p_frame.cols,
  //       p_frame.rows,
  //       (int)p_frame.step,
  //       QImage::Format::Format_Indexed8};
  //   fileImage(k_image.copy());
  // }
  // if (!image.isNull()) {
  //   // auto k_image = image;
  // }
}

PlayerMotion::PlayerMotion()
    : p_mutex(),
      p_condition(),
      p_thread(),
      p_thread_stop(false),
      p_stop_player(true),
      p_video_file(),
      p_video(std::make_shared<cv::VideoCapture>()),
      p_frame(),
      p_delay(1) {
  p_thread = std::thread{[this] {
    //这个已经是在其他线程中调用了
    for (;;) {
      {
        std::unique_lock<std::mutex> lock(this->p_mutex);
        this->p_condition.wait(lock, [=] { return !this->p_stop_player || this->p_thread_stop; });
        if (this->p_thread_stop)
          return;
      }
      this->readFrame();
      std::this_thread::sleep_for(std::chrono::milliseconds{p_delay});
    }
  }};
}

void PlayerMotion::startPlayer() {
  if (!FSys::exists(p_video_file.generic_wstring()))
    throw NotFileError(p_video_file);

  {  //开始获得锁
    std::unique_lock<std::mutex> lock(p_mutex);
    if (!p_video->open(p_video_file.generic_string()))
      throw MotionError("not open file " + p_video_file.generic_u8string());

    auto fps = p_video->get(cv::CAP_PROP_FPS);
    p_delay  = 1000 / fps;

    p_stop_player = false;
  }

  p_condition.notify_one();
}

void PlayerMotion::stop_Player() {
  {
    std::unique_lock<std::mutex> lock(p_mutex);
    p_stop_player = true;
    if (this->p_video->isOpened())
      this->p_video->release();
  }
  p_condition.notify_one();
}

void PlayerMotion::setFile(const FSys::path &file) {
  {
    std::unique_lock<std::mutex> lock(p_mutex);
    p_video_file = file;
  }
  startPlayer();
}

PlayerMotion::~PlayerMotion() {
  {
    std::lock_guard<std::mutex> lock(this->p_mutex);
    p_stop_player = false;
    p_thread_stop = true;
  }
  p_condition.notify_all();
  p_thread.join();
}
}  // namespace doodle::motion::kernel